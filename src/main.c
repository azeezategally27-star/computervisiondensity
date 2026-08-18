#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <opencv2/opencv.h>
#include "detector_fake.h"
#include "detector_onnx.h"
#include "tracker.h"
#include "kde.h"
#include "roi.h"
#include "utils.h"
#include <json-c/json.h>

#define MAX_DETS 512

static void load_config(const char* fname, Polygon* poly, int* roi_threshold, double* scale, double* sigma){
    FILE* f = fopen(fname,"r");
    if(!f){ fprintf(stderr,"config.json not found, using defaults\n"); poly->n=0; *roi_threshold=5; *scale=0.25; *sigma=20.0; return; }
    fseek(f,0,SEEK_END); long sz = ftell(f); fseek(f,0,SEEK_SET);
    char* buf = malloc(sz+1); fread(buf,1,sz,f); buf[sz]=0; fclose(f);
    struct json_object* obj = json_tokener_parse(buf);
    if(obj){
        struct json_object* jroi = NULL;
        if(json_object_object_get_ex(obj,"roi",&jroi)){
            int n = json_object_array_length(jroi);
            poly->n = n;
            poly->xs = malloc(sizeof(int)*n);
            poly->ys = malloc(sizeof(int)*n);
            for(int i=0;i<n;i++){
                struct json_object* pt = json_object_array_get_idx(jroi,i);
                if(json_object_array_length(pt)>=2){
                    poly->xs[i] = json_object_get_int(json_object_array_get_idx(pt,0));
                    poly->ys[i] = json_object_get_int(json_object_array_get_idx(pt,1));
                } else {
                    poly->xs[i]=0; poly->ys[i]=0;
                }
            }
        }
        struct json_object* thr=NULL;
        if(json_object_object_get_ex(obj,"roi_threshold",&thr)) *roi_threshold = json_object_get_int(thr);
        struct json_object* s=NULL;
        if(json_object_object_get_ex(obj,"heatmap_scale",&s)) *scale = json_object_get_double(s);
        struct json_object* sg=NULL;
        if(json_object_object_get_ex(obj,"kernel_sigma",&sg)) *sigma = json_object_get_double(sg);
        json_object_put(obj);
    }
    free(buf);
}

int main(int argc, char** argv){
    const char* source = "0";
    const char* cfg = "config.json";
    int use_onnx = 0;
    const char* model_path = "yolov8.onnx";
    for(int i=1;i<argc;i++){
        if(strcmp(argv[i],"--source")==0 && i+1<argc) source = argv[++i];
        if(strcmp(argv[i],"--config")==0 && i+1<argc) cfg = argv[++i];
        if(strcmp(argv[i],"--use-onnx")==0 && i+1<argc) use_onnx = atoi(argv[++i]);
        if(strcmp(argv[i],"--model")==0 && i+1<argc) model_path = argv[++i];
    }
    Polygon roi = {0};
    int roi_threshold = 5;
    double heatmap_scale = 0.25;
    double kernel_sigma = 20.0;
    load_config(cfg, &roi, &roi_threshold, &heatmap_scale, &kernel_sigma);

    if(use_onnx){
        if(onnx_init(model_path)!=0){
            fprintf(stderr,"Failed to initialize ONNX model at %s\n", model_path);
            return -1;
        }
    }

    CvCapture* cap = NULL;
    int cam_index = -1;
    if(strlen(source)==1 && source[0]>='0' && source[0]<='9'){
        cam_index = atoi(source);
        cap = cvCaptureFromCAM(cam_index);
    } else {
        cap = cvCaptureFromFile(source);
    }
    if(!cap){
        fprintf(stderr,"Could not open source '%s'\n", source);
        return -1;
    }
    IplImage* frame = NULL;
    Tracker* tracker = tracker_create();

    double last_t = now_seconds();
    int frame_count = 0;
    while(1){
        frame = cvQueryFrame(cap);
        if(!frame) break;
        int width = frame->width, height = frame->height;
        int det_x[MAX_DETS], det_y[MAX_DETS], det_w[MAX_DETS], det_h[MAX_DETS];
        int ndet = 0;
        // detection
        if(use_onnx){
            onnx_detect(frame, &ndet, det_x, det_y, det_w, det_h);
        } else {
            fake_detect(frame, &ndet, det_x, det_y, det_w, det_h);
        }

        // compute centroids list for KDE and ROI checks
        int xs[MAX_DETS], ys[MAX_DETS];
        for(int i=0;i<ndet;i++){ xs[i] = det_x[i] + det_w[i]/2; ys[i] = det_y[i] + det_h[i]/2; }

        // update tracker
        tracker_update(tracker, ndet, det_x, det_y, det_w, det_h);

        // compute heatmap
        IplImage* heatColor = compute_heatmap(width, height, ndet, xs, ys, kernel_sigma, heatmap_scale);
        // overlay heatmap onto frame
        // resize heatColor to frame size
        IplImage* heatResized = cvCreateImage(cvSize(width,height), IPL_DEPTH_8U, 3);
        cvResize(heatColor, heatResized, CV_INTER_LINEAR);
        // blend
        cvAddWeighted(frame, 0.6, heatResized, 0.4, 0, frame);
        cvReleaseImage(&heatColor);
        cvReleaseImage(&heatResized);

        // draw detections and IDs
        for(int i=0;i<tracker->n;i++){
            Track* tr = &tracker->tracks[i];
            CvPoint p1 = cvPoint(tr->x, tr->y);
            CvPoint p2 = cvPoint(tr->x+tr->w, tr->y+tr->h);
            cvRectangle(frame, p1, p2, CV_RGB(0,255,0), 2, 8, 0);
            char idtext[32]; snprintf(idtext,32,"ID:%d", tr->id);
            draw_text(frame, idtext, tr->x, tr->y-5, CV_RGB(0,255,0));
        }

        // check ROI counts
        int in_roi = 0;
        for(int i=0;i<tracker->n;i++){
            int cx = tracker->tracks[i].x + tracker->tracks[i].w/2;
            int cy = tracker->tracks[i].y + tracker->tracks[i].h/2;
            if(point_in_poly(cx, cy, &roi)) in_roi++;
        }
        // draw ROI polygon
        for(int i=0;i<roi.n;i++){
            CvPoint a = cvPoint(roi.xs[i], roi.ys[i]);
            CvPoint b = cvPoint(roi.xs[(i+1)%roi.n], roi.ys[(i+1)%roi.n]);
            CvScalar color = (in_roi > roi_threshold) ? CV_RGB(255,0,0) : CV_RGB(255,255,0);
            cvLine(frame, a, b, color, 2, 8, 0);
        }
        if(in_roi > roi_threshold){
            draw_text(frame, "ALERT: ROI THRESHOLD EXCEEDED", 10, 30, CV_RGB(255,0,0));
        }

        // telemetry
        frame_count++;
        double tnow = now_seconds();
        if(tnow - last_t >= 1.0){
            double fps = frame_count / (tnow - last_t);
            char buf[128];
            snprintf(buf, sizeof(buf), "FPS: %.1f  Total: %d  ROI: %d", fps, tracker->n, in_roi);
            draw_text(frame, buf, 10, frame->height-10, CV_RGB(255,255,255));
            last_t = tnow; frame_count = 0;
        } else {
            char buf2[128];
            snprintf(buf2, sizeof(buf2), "Total: %d  ROI: %d", tracker->n, in_roi);
            draw_text(frame, buf2, 10, frame->height-10, CV_RGB(255,255,255));
        }

        cvShowImage("Crowd Monitor", frame);
        int key = cvWaitKey(1);
        if(key==27) break;
    }

    tracker_destroy(tracker);
    if(roi.n) polygon_free(&roi);
    cvReleaseCapture(&cap);
    if(use_onnx) onnx_release();
    return 0;
}
