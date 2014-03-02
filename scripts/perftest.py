#!/usr/bin/env python
# -*- coding: utf-8 -*-

import json
import subprocess
import time
import psutil

def get_process_info(p):
    try:
        cpu = int(p.get_cpu_percent(interval=0)) #0: non-blocking
        rss, vms = p.get_memory_info()
        name = p.name
        pid = p.pid
    except psutil.error.NoSuchProcess, e:
        name = "Invalid process"
        pid = 0
        rss = 0
        vms = 0
        cpu = 0
    return {'cpu': cpu, 'rss': rss, 'vms': vms}

def main():
    conf_file = open('perftest.json')
    conf = json.load(conf_file)
    conf_file.close()
    cmd = conf['cmd']
    # shell == True: process is shell process
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=False)
    p = psutil.Process(proc.pid)
    if not p.is_running():
        return
    name = p.name
    pid = p.pid
    record = []
    print 'recoding...'
    while True:
        if not p.is_running():
            break
        record.append(get_process_info(p))
        time.sleep(1)

    proc_history = { 'name': name, 'pid': pid, 'history': record }
    print proc_history
    with open('perftest_result.json', 'wb') as fp:
        json.dump(proc_history, fp)
    print 'result is saved in perftest_result.json'

if __name__ == '__main__':
    main()
