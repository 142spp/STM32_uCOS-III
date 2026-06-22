#ifndef  TASK_BUTTON_H
#define  TASK_BUTTON_H

/* 출차 버튼을 디바운싱하여 ManagerQueue(MSG_EXIT)로 출차 이벤트 전송 */
void  ButtonTask_Create(void);

/* 버튼 ISR이 호출하여 task를 깨운다 (ISR은 sem post만 수행) */
void  ButtonTask_SignalFromISR(void);

#endif  /* TASK_BUTTON_H */
