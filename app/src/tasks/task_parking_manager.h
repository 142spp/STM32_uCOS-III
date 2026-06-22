#ifndef  TASK_PARKING_MANAGER_H
#define  TASK_PARKING_MANAGER_H

/*
 * 시스템 상태의 단일 소유자(single owner).
 * 주차칸 상태/빈자리 수/차단기 상태는 이 task만 변경하므로 별도 락이 필요 없다.
 * ManagerQueue 하나만 기다리며 모든 입력(센서/입차/출차/타이머)을 직렬 처리한다.
 */
void  ParkingManagerTask_Create(void);

#endif  /* TASK_PARKING_MANAGER_H */
