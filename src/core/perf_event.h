/*
Copyright (c) 2018 Viktor Leis
Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:
The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once
#include <iostream>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>
#include <cstring>

// 성능 카운터 이벤트 설정 함수
int setup_perf_event(int perf_type, int perf_config) {
    struct perf_event_attr pe;
    memset(&pe, 0, sizeof(struct perf_event_attr));
    pe.type = perf_type;
    pe.size = sizeof(struct perf_event_attr);
    pe.config = perf_config;
    pe.disabled = 1;  // 시작 시 비활성화
    pe.exclude_kernel = 1;  // 커널 모드 제외
    pe.exclude_hv = 1;  // 하이퍼바이저 모드 제외

    int fd = syscall(__NR_perf_event_open, &pe, 0, -1, -1, 0);
    if (fd == -1) {
        std::cerr << "Error opening perf event: " << strerror(errno) << std::endl;
    }
    return fd;
}

// 성능 카운터 파일 디스크립터를 열고 재사용
void initialize_perf_events() {
    fd_llc_miss = setup_perf_event(PERF_TYPE_HW_CACHE, 
                                   (PERF_COUNT_HW_CACHE_LL) |
                                   (PERF_COUNT_HW_CACHE_OP_READ << 8) |
                                   (PERF_COUNT_HW_CACHE_RESULT_MISS << 16));
    fd_dtlb_miss = setup_perf_event(PERF_TYPE_HW_CACHE, 
                                    (PERF_COUNT_HW_CACHE_DTLB) |
                                    (PERF_COUNT_HW_CACHE_OP_READ << 8) |
                                    (PERF_COUNT_HW_CACHE_RESULT_MISS << 16));
    fd_branch_miss = setup_perf_event(PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_MISSES);
    fd_instructions = setup_perf_event(PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS);
}

// 성능 카운터 시작
void start_perf_event(int fd) {
    if (fd != -1) {
        ioctl(fd, PERF_EVENT_IOC_RESET, 0);
        ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
    }
}

// 성능 카운터 종료 및 결과 가져오기
long long stop_perf_event(int fd) {
    if (fd != -1) {
        ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
        long long count;
        if (read(fd, &count, sizeof(long long)) == -1) {
            std::cerr << "Error reading perf event: " << strerror(errno) << std::endl;
            return 0;
        }
        return count;
    }
    return 0;
}

// 성능 카운터 파일 디스크립터 닫기
void close_perf_events() {
    if (fd_llc_miss != -1) close(fd_llc_miss);
    if (fd_dtlb_miss != -1) close(fd_dtlb_miss);
    if (fd_branch_miss != -1) close(fd_branch_miss);
    if (fd_instructions != -1) close(fd_instructions);
}

