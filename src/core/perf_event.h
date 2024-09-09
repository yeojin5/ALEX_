#ifndef PERF_EVENT_H
#define PERF_EVENT_H

#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/syscall.h>

// perf_event_open 함수 정의
static long perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                            int cpu, int group_fd, unsigned long flags)
{
    return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

// 성능 이벤트 파일 디스크립터 구조체
typedef struct {
    int fd_cycles;
    int fd_instructions;
} perf_fds;

// 성능 이벤트 초기화 함수
perf_fds init_perf_events() {
    struct perf_event_attr pe_cycles, pe_instructions;
    perf_fds fds;

    // 1. CPU 사이클 이벤트 설정
    memset(&pe_cycles, 0, sizeof(struct perf_event_attr));
    pe_cycles.type = PERF_TYPE_HARDWARE;
    pe_cycles.size = sizeof(struct perf_event_attr);
    pe_cycles.config = PERF_COUNT_HW_CPU_CYCLES;
    pe_cycles.disabled = 1;  // 시작 시 비활성화
    pe_cycles.exclude_kernel = 1;  // 커널 모드 제외
    pe_cycles.exclude_hv = 1;      // 하이퍼바이저 모드 제외

    // 2. CPU 사이클 이벤트로 그룹의 리더 설정
    fds.fd_cycles = perf_event_open(&pe_cycles, 0, -1, -1, 0);
    if (fds.fd_cycles == -1) {
        fprintf(stderr, "Error opening CPU cycles event\n");
        exit(EXIT_FAILURE);
    }

    // 3. 명령어 실행 이벤트 설정
    memset(&pe_instructions, 0, sizeof(struct perf_event_attr));
    pe_instructions.type = PERF_TYPE_HARDWARE;
    pe_instructions.size = sizeof(struct perf_event_attr);
    pe_instructions.config = PERF_COUNT_HW_INSTRUCTIONS;
    pe_instructions.disabled = 1;  // 시작 시 비활성화
    pe_instructions.exclude_kernel = 1;  // 커널 모드 제외
    pe_instructions.exclude_hv = 1;      // 하이퍼바이저 모드 제외

    // 4. 명령어 실행 이벤트를 사이클 이벤트 그룹에 추가
    fds.fd_instructions = perf_event_open(&pe_instructions, 0, -1, fds.fd_cycles, 0);
    if (fds.fd_instructions == -1) {
        fprintf(stderr, "Error opening Instructions event\n");
        exit(EXIT_FAILURE);
    }

    return fds;
}

// 성능 카운터 시작
void start_perf_events(perf_fds *fds) {
    ioctl(fds->fd_cycles, PERF_EVENT_IOC_RESET, 0);
    ioctl(fds->fd_instructions, PERF_EVENT_IOC_RESET, 0);
    ioctl(fds->fd_cycles, PERF_EVENT_IOC_ENABLE, 0);  // 리더 이벤트 활성화
}

// 성능 카운터 종료 및 결과 읽기
void stop_and_read_perf_events(perf_fds *fds) {
    long long count_cycles, count_instructions;

    ioctl(fds->fd_cycles, PERF_EVENT_IOC_DISABLE, 0);  // 리더 이벤트 비활성화

    read(fds->fd_cycles, &count_cycles, sizeof(long long));
    read(fds->fd_instructions, &count_instructions, sizeof(long long));

    printf("CPU cycles: %lld\n", count_cycles);
    printf("Instructions: %lld\n", count_instructions);
}

// 성능 이벤트 닫기
void close_perf_events(perf_fds *fds) {
    close(fds->fd_cycles);
    close(fds->fd_instructions);
}

#endif

