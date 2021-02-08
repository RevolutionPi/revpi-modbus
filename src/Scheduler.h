/*!
 *
 * Project: piModbusSlave
 * (C)    : KUNBUS GmbH, Heerweg 15C, 73370 Denkendorf, Germany
 *
 */

#ifndef MODBUS_SCHEDULER_H_
#define MODBUS_SCHEDULER_H_

#if (_POSIX_C_SOURCE < 199309L)
#define _POSIX_C_SOURCE 199309L //define for struct timespec
#endif


#include <time.h>
#include <sys/queue.h>
#include "modbusconfig.h"

extern const int32_t s32_microseconds_per_second;
extern const int32_t s32_nanoseconds_per_second;

/************************************************************************/
/** @ brief struct which is used by the modbus actions scheduler
 *  
 *	contains a pointer to the modbus action of type TModbusAction*,
 *		the interval time of the modbus action,
 *		the remaining time until the action has to be processed again
 *		and the TAILQ_ENTRY which contains the pointers for the double linked list
 */
/************************************************************************/
struct schedulerEvent
{
	struct timespec intervalTime;
	struct timespec triggerTime;
	TModbusAction* ptModbusAction;
	TAILQ_ENTRY(schedulerEvent) events;
};


/************************************************************************/
/** @ brief struct which is used as return value for the modbus function processor
 *  
 *	contains a pointer to the modbus action of type TModbusAction*,
 *	the absolute trigger time for the event
 */
/************************************************************************/
typedef struct
{
	struct timespec triggerTime;
	TModbusAction* ptModbusAction;
} tModbusEvent;

/************************************************************************/
/** @ brief struct for the scheduler event list head
 *  
 */
/************************************************************************/
TAILQ_HEAD(suEventListHead, schedulerEvent);


int32_t initScheduler(struct TMBActionListHead tModbusActionListHead_p, struct suEventListHead *pEventListHead_p);
void cleanupScheduler(struct suEventListHead *pEventListHead_p);
int32_t getNextEvent(tModbusEvent* next_modbus_event_p, struct suEventListHead *pEventListHead_p);
int32_t getNextEventAndTimeout(tModbusEvent* next_modbus_event_p, const struct timespec *time_elapsed_p, struct timespec *max_timeout_p, struct suEventListHead *pEventListHead_p);
void determineNextEvent(tModbusEvent* nextEvent, struct suEventListHead *pEventListHead_p);
int32_t determineNextEvent_relativeTime(tModbusEvent* next_modbus_event_p, const struct timespec *timeElapsed_p, struct timespec *max_timeout_p, struct suEventListHead *pEventListHead_p);
void insertEventByDueDate(struct schedulerEvent* newEvent, struct suEventListHead *pEventListHead_p);
void get_minimal_modbus_action_interval(struct timespec* min_interval_p, struct suEventListHead *pEventListHead_p);
void get_minimal_modbus_event_offset(struct timespec* min_event_offset_p, struct suEventListHead *pEventListHead_p);


int32_t timespec_diff(struct timespec *diff, const struct timespec *minuend, const struct timespec *subtrahend);
int32_t timespec_add(struct timespec *sum, const struct timespec *summand1, const struct timespec *summand2);


#endif /* MODBUS_SCHEDULER_H_ */
