#!/usr/bin/env python3
"""
Pygame frontend for the Pixel Agents Mauritian Airport demo.
- Pan the large world with arrow keys or WASD.
- Click an agent to fetch its tasks and perform the first task.
- Visual feedback: agent flashes while performing a task and a log entry appears.
"""

import pygame, sys, socket, json, threading, time
from pygame.locals import *

HOST = '127.0.0.1'
PORT = 9191

WORLD_W, WORLD_H = 2400, 1200
WINDOW_W, WINDOW_H = 1000, 700
AGENT_SIZE = 12  # pixel-sized square (scaled up)
SCALE = 2

pygame.init()
font = pygame.font.SysFont('Consolas', 14)
screen = pygame.display.set_mode((WINDOW_W, WINDOW_H))
pygame.display.set_caption('Pixel Agents — Mauritian Airport')
clock = pygame.time.Clock()

# simple tcp helper
def send_cmd(cmd):
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(1.0)
        s.connect((HOST, PORT))
        s.sendall((cmd + "\n").encode('utf-8'))
        data = b''
        while True:
            part = s.recv(4096)
            if not part: break
            data += part
        s.close()
        return data.decode('utf-8')
    except Exception as e:
        return None

# load agents from backend
agents = []
agent_tasks = {}
agent_state = {}  # will store animation / performing state

def load_agents():
    global agents
    res = send_cmd('LIST_AGENTS')
    if not res:
        print('Failed to connect to backend')
        return
    # crude parse: AGENTS [ {"id":1,...}, {...} ]
    start = res.find('[')
    end = res.rfind(']')
    if start==-1 or end==-1: return
    body = res[start+1:end]
    # split top-level objects
    objs = []
    cur = ''
    depth = 0
    for ch in body:
        cur += ch
        if ch=='{': depth+=1
        if ch=='}': depth-=1
        if depth==0 and cur.strip(): objs.append(cur.strip()); cur=''
    parsed = []
    for o in objs:
        o = o.strip('{}')
        d = {}
        parts = o.split(',')
        for p in parts:
            if ':' in p:
                k,v = p.split(':',1)
                k=k.strip().strip('"')
                v=v.strip().strip('"')
                d[k]=v
        parsed.append(d)
    agents = []
    for p in parsed:
        a = { 'id': int(p['id']), 'name': p['name'], 'role': p['role'], 'x': int(p['x']), 'y': int(p['y']) }
        agents.append(a)
        agent_state[a['id']] = {'performing': False, 'timer':0, 'color': (200,80,180)}

load_agents()

# general UI state
cam_x, cam_y = 0, 0
log_lines = []

# request tasks when clicking
def fetch_tasks(agent_id):
    res = send_cmd(f'TASKS {agent_id}')
    if not res: return []
    if not res.startswith('TASKS'): return []
    # TASKS id ["t1","t2"]
    bstart = res.find('['); bend = res.rfind(']')
    if bstart==-1 or bend==-1: return []
    content = res[bstart+1:bend]
    items = []
    cur = ''
    in_str = False
    for ch in content:
        if ch=='"': in_str = not in_str
        elif ch==',' and not in_str:
            if cur.strip(): items.append(cur.strip().strip('"'))
            cur=''
        else:
            cur += ch
    if cur.strip(): items.append(cur.strip().strip('"'))
    return items

# perform task via backend; will update agent_state
def perform_task(agent_id, task_index):
    res = send_cmd(f'PERFORM_TASK {agent_id} {task_index}')
    if not res:
        log(f'Backend unreachable for PERFORM_TASK')
        return
    if res.startswith('TASK_STARTED'):
        parts = res.split()
        aid = int(parts[1]); tid = int(parts[2]); delay = int(parts[3])
        # set performing state
        agent_state[aid]['performing'] = True
        agent_state[aid]['timer'] = delay
        agent_state[aid]['color'] = (255,180,80)
        log(f'Agent {aid} started task {tid} (duration {delay}ms)')
        # spawn a thread to clear state after delay
        def _wait_clear(aid,delay):
            time.sleep(delay/1000.0)
            agent_state[aid]['performing'] = False
            agent_state[aid]['color'] = (80,200,160)
            log(f'Agent {aid} completed task {tid}')
        t = threading.Thread(target=_wait_clear, args=(aid,delay), daemon=True)
        t.start()
    else:
        log('Task request failed: ' + res)

# simple logging
def log(msg):
    timestamp = time.strftime('%H:%M:%S')
    entry = f'[{timestamp}] {msg}'
    print(entry)
    log_lines.insert(0, entry)
    if len(log_lines) > 8: log_lines.pop()

# initial colors
for a in agents:
    agent_state[a['id']]['color'] = (80,200,160)

# create many additional agents procedurally for density
next_id = max([a['id'] for a in agents]) if agents else 0
roles = ['Check-in Agent','Security Officer','Baggage Handler','Gate Agent','Customer Service','Cleaning','Customs','Immigration','Ground Crew']
names = ['Anna','Babu','Celine','Dinesh','Elise','Farah','Gino','Hana','Ibrahim','Joao','Karla','Lars','Moussa','Nora','Omar','Priya','Quinn','Ravi','Siti','Tariq']
import random
for i in range(180):
    next_id += 1
    x = random.randint(50, WORLD_W-50)
    y = random.randint(50, WORLD_H-50)
    role = random.choice(roles)
    name = random.choice(names) + str(random.randint(1,99))
    agents.append({'id': next_id, 'name': name, 'role': role, 'x': x, 'y': y})
    agent_state[next_id] = {'performing': False, 'timer':0, 'color': (100,200,200)}

# main loop
running = True
selected_agent = None
selected_tasks = []
selected_task_idx = 0

while running:
    dt = clock.tick(60)
    for event in pygame.event.get():
        if event.type == QUIT: running = False
        elif event.type == KEYDOWN:
            if event.key == K_ESCAPE: running = False
            if event.key == K_LEFT or event.key == K_a: cam_x = max(0, cam_x-50)
            if event.key == K_RIGHT or event.key == K_d: cam_x = min(WORLD_W - WINDOW_W, cam_x+50)
            if event.key == K_UP or event.key == K_w: cam_y = max(0, cam_y-50)
            if event.key == K_DOWN or event.key == K_s: cam_y = min(WORLD_H - WINDOW_H, cam_y+50)
            if event.key == K_r: load_agents(); log('Reloaded agents from backend')
        elif event.type == MOUSEBUTTONDOWN:
            mx,my = event.pos
            world_x = cam_x + mx
            world_y = cam_y + my
            # find clicked agent within radius
            clicked = None
            for a in agents:
                ax = a['x']; ay = a['y']
                if abs(ax - world_x) < 12 and abs(ay - world_y) < 12:
                    clicked = a; break
            if clicked:
                selected_agent = clicked
                selected_tasks = fetch_tasks(clicked['id'])
                selected_task_idx = 0
                log(f'Clicked on {clicked["name"]} ({clicked["role"]})')
                if selected_tasks:
                    # perform first task automatically
                    perform_task(clicked['id'], 0)

    # update
    for aid,st in agent_state.items():
        if st['performing'] and st['timer']>0:
            st['timer'] -= dt
            # blink
            if (st['timer']//150) % 2 == 0:
                st['color'] = (255,200,80)
            else:
                st['color'] = (255,120,80)

    # draw world
    screen.fill((30,30,40))
    # draw runway/areas background as simple rectangles
    pygame.draw.rect(screen, (70,70,90), (-cam_x, -cam_y, WINDOW_W, WINDOW_H))

    # draw a grid for scale
    for gx in range(0, WORLD_W, 100):
        sx = gx - cam_x
        pygame.draw.line(screen, (20,20,30), (sx, -cam_y), (sx, WORLD_H-cam_y))
    for gy in range(0, WORLD_H, 100):
        sy = gy - cam_y
        pygame.draw.line(screen, (20,20,30), (-cam_x, sy), (WORLD_W-cam_x, sy))

    # draw agents
    for a in agents:
        sx = a['x'] - cam_x; sy = a['y'] - cam_y
        if sx < -20 or sx > WINDOW_W+20 or sy < -20 or sy > WINDOW_H+20: continue
        st = agent_state[a['id']]
        color = st.get('color', (150,220,150))
        rect = pygame.Rect(sx-AGENT_SIZE, sy-AGENT_SIZE, AGENT_SIZE*2, AGENT_SIZE*2)
        pygame.draw.rect(screen, color, rect)
        # tiny pixel eye
        pygame.draw.rect(screen, (10,10,10), (sx-4, sy-4, 3,3))
        # name label if near center
        if WINDOW_W/3 < sx < 2*WINDOW_W/3 and WINDOW_H/3 < sy < 2*WINDOW_H/3:
            txt = font.render(a['name'], True, (240,240,240))
            screen.blit(txt, (sx+10, sy-10))

    # side panel
    panel_rect = pygame.Rect(WINDOW_W-260, 10, 250, 220)
    pygame.draw.rect(screen, (18,18,28), panel_rect)
    pygame.draw.rect(screen, (120,80,200), (WINDOW_W-260, 10, 250, 30))
    if selected_agent:
        txt1 = font.render(f"Selected: {selected_agent['name']}", True, (255,255,255))
        screen.blit(txt1, (WINDOW_W-250, 15))
        roletxt = font.render(f"Role: {selected_agent['role']}", True, (200,200,220))
        screen.blit(roletxt, (WINDOW_W-250, 40))
        # tasks list
        for i,tsk in enumerate(selected_tasks[:6]):
            tcol = (200,200,200) if i!=selected_task_idx else (255,220,100)
            txt = font.render(f"{i}. {tsk}", True, tcol)
            screen.blit(txt, (WINDOW_W-250, 70 + i*22))
    else:
        txt = font.render("Click an agent to interact", True, (220,220,220))
        screen.blit(txt, (WINDOW_W-250, 20))

    # bottom log
    for i,ln in enumerate(log_lines[:6]):
        txt = font.render(ln, True, (220,220,220))
        screen.blit(txt, (10, WINDOW_H-20 - i*18))

    # mini-map
    mm_w, mm_h = 220, 110
    mm_x, mm_y = 10, 10
    pygame.draw.rect(screen, (10,10,18), (mm_x, mm_y, mm_w, mm_h))
    # draw agents as dots scaled
    for a in agents[:400]:
        dot_x = mm_x + int(a['x'] * mm_w / WORLD_W)
        dot_y = mm_y + int(a['y'] * mm_h / WORLD_H)
        pygame.draw.rect(screen, (120,200,120), (dot_x, dot_y, 2,2))
    # viewport rectangle
    vx = mm_x + int(cam_x * mm_w / WORLD_W)
    vy = mm_y + int(cam_y * mm_h / WORLD_H)
    vw = int(WINDOW_W * mm_w / WORLD_W)
    vh = int(WINDOW_H * mm_h / WORLD_H)
    pygame.draw.rect(screen, (255,255,255), (vx,vy,vw,vh), 1)

    pygame.display.flip()

pygame.quit()
sys.exit()
