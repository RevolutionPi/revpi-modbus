/*!
 *
 * Project: piModbusSlave
 * (C)    : KUNBUS GmbH, Heerweg 15C, 73370 Denkendorf, Germany
 *
 */

#include <project.h>

#include <Scheduler.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <syslog.h>

//#define SCHEDULER_DEBUG


const int32_t s32_microseconds_per_second   = 1000000;
const int32_t s32_nanoseconds_per_second    = 1000000000;



/************************************************************************/
/** @ brief initializes the modbus action scheduler
 *  
 *  @param paModbusActions_p pointer to all modbus actions for this instance
 *  @param pEventListHead_p pointer to the event list head
 *  @return returns '0' if initialisation was successful otherwise '-1'
 *  
 */
/************************************************************************/
int32_t initScheduler(struct TMBActionListHead tModbusActionListHead_p, struct suEventListHead *pEventListHead_p)
{
    struct schedulerEvent* pNewSchedulerEvent;
    struct TMBActionEntry* nextModbusAction = NULL;
    if (SLIST_EMPTY(&tModbusActionListHead_p))
    {
        syslog(LOG_ERR, "No modbus actions for device");
        return -1;
    }
    TAILQ_INIT(pEventListHead_p);
    //get absolute system time to determine trigger time for all events
    struct timespec tv_currentTime;
    clock_gettime(CLOCK_MONOTONIC, &tv_currentTime);
    
    SLIST_FOREACH(nextModbusAction, &tModbusActionListHead_p, entries)
    {
        pNewSchedulerEvent = malloc(sizeof(struct schedulerEvent));
        if (pNewSchedulerEvent == NULL)
        {
            //if malloc of one element fails, delete the whole list and free memory
            syslog(LOG_ERR, "Could not initialize modbus command scheduler. Memory allocation failed");
            cleanupScheduler(pEventListHead_p);
            return -1;
        }
        else
        {
            pNewSchedulerEvent->ptModbusAction = &(nextModbusAction->modbusAction);	
            pNewSchedulerEvent->intervalTime.tv_sec  = ((pNewSchedulerEvent->ptModbusAction->i32uInterval_us) / s32_microseconds_per_second);
            pNewSchedulerEvent->intervalTime.tv_nsec = ((pNewSchedulerEvent->ptModbusAction->i32uInterval_us) % s32_microseconds_per_second) * 1000;
            //trigger time is absolute time plus interval Time plus an additional second for initialisation
            timespec_add(&(pNewSchedulerEvent->triggerTime), &tv_currentTime, &(pNewSchedulerEvent->intervalTime));
            //md timespec_add(&(pNewSchedulerEvent->triggerTime), &tv_currentTime, &(pNewSchedulerEvent->intervalTime));
            //additional second as buffer for initialisation
            pNewSchedulerEvent->triggerTime.tv_sec = pNewSchedulerEvent->triggerTime.tv_sec + 1;
            
            //insert new entry by due date
            if (TAILQ_EMPTY(pEventListHead_p))
            {
                //first element
                TAILQ_INSERT_HEAD(pEventListHead_p, pNewSchedulerEvent, events);
            }
            else
            {
                insertEventByDueDate(pNewSchedulerEvent, pEventListHead_p);
            }
            
        }	
    }
    
#ifdef SCHEDULER_DEBUG
    struct schedulerEvent* pEvent = NULL;
    TAILQ_FOREACH(pEvent, pEventListHead_p, events)
    {
        syslog(LOG_INFO, "Modbus action list entry: %d, %d, %d, %d.%06ds, %d, %d, %d\n",
            pEvent->ptModbusAction->i8uSlaveAddress,
            pEvent->ptModbusAction->eFunctionCode,
            pEvent->ptModbusAction->i32uStartRegister,
            (int)(pEvent->triggerTime.tv_sec),
            (int)(pEvent->triggerTime.tv_nsec/1000),
            pEvent->ptModbusAction->i32uInterval_us,
            pEvent->ptModbusAction->i32uStartByteProcessData,
            pEvent->ptModbusAction->i8uStartBitProcessData);
    }
#endif
    
    return 0;
}


void cleanupScheduler(struct suEventListHead *pEventListHead_p)
{
    struct schedulerEvent* pNewSchedulerEvent;
 
    while (!TAILQ_EMPTY(pEventListHead_p))
    {
        pNewSchedulerEvent = (struct schedulerEvent*)TAILQ_FIRST(pEventListHead_p);				
        TAILQ_REMOVE(pEventListHead_p, pNewSchedulerEvent, events);
        free(pNewSchedulerEvent);
    }
}


/************************************************************************/
/** @ brief get event (modbus action) which has to be processed next
 *  
 *  @param[out] pointer to the next modbus event which has to be processed
 *  @return '0' if successful, otherwise '-1'
 *  
 */
/************************************************************************/
int32_t getNextEvent(tModbusEvent* next_modbus_event_p, struct suEventListHead *pEventListHead_p)
{
    if (TAILQ_EMPTY(pEventListHead_p))
    {
        syslog(LOG_ERR, "No entries in modbus action list");
        return -1;
    }
    
    determineNextEvent(next_modbus_event_p, pEventListHead_p);
    return 0;
}

/************************************************************************/
/** @ brief get event (modbus action) which has to be processed next
 *  
 *  @param[in] time_elapsed_p processing time of the previous modbus action to adjust the timing of all following modbus commands
 *	@param[out] max_timeout_p maximal calculated timeout for all actions in the modbus action event list
 *  @return pointer to the next modbus event which has to be processed
 *  
 */
/************************************************************************/
int32_t getNextEventAndTimeout(tModbusEvent* next_modbus_event_p, const struct timespec *time_elapsed_p, struct timespec *max_timeout_p, struct suEventListHead *pEventListHead_p)
{
//#error  Modbus action timeouts are not handled appropriate
    if (TAILQ_EMPTY(pEventListHead_p))
    {
        syslog(LOG_ERR, "No entries in modbus action list");
        return -1;
    }
    else
    {
        determineNextEvent_relativeTime(next_modbus_event_p, time_elapsed_p, max_timeout_p, pEventListHead_p);
    }
    return 0;
}


/************************************************************************/
/** @ brief determines the next modbus event 
  *     Should only be invoked by function getNextEvent()
 *  
 *  @param[in]	time_elapsed_p processing time of the previous modbus action to adjust the timing of all following modbus commands
  *	@param[out] max_timeout_p maximum timeout value of all modbus actions in event list
 *  @return maximal timeout of all modbus actions
 *  
 */
/************************************************************************/
void determineNextEvent(tModbusEvent* next_modbus_event_p, struct suEventListHead *pEventListHead_p)
{
    struct schedulerEvent* pNextSchedulerEvent_l = NULL;

    //store scheduler event with earliest due date and next modbus event
    pNextSchedulerEvent_l = TAILQ_FIRST(pEventListHead_p);
    next_modbus_event_p->triggerTime = pNextSchedulerEvent_l->triggerTime;
    next_modbus_event_p->ptModbusAction = pNextSchedulerEvent_l->ptModbusAction;
    //remove entry with earliest due date from list
    TAILQ_REMOVE(pEventListHead_p, pNextSchedulerEvent_l, events);	
    //add interval time to removed scheduler event
    timespec_add(&(pNextSchedulerEvent_l->triggerTime), &(pNextSchedulerEvent_l->triggerTime), &(pNextSchedulerEvent_l->intervalTime));
    //insert entry according to the new due date
    insertEventByDueDate(pNextSchedulerEvent_l, pEventListHead_p);	
}

/************************************************************************/
/** @ brief determines the next modbus event 
  *     Should only be invoked by function getNextEvent()
 *  
 *  @param[in]	time_elapsed_p processing time of the previous modbus action to adjust the timing of all following modbus commands
  *	@param[out] max_timeout_p maximum timeout value of all modbus actions in event list
 *  @return maximal timeout of all modbus actions
 *  
 */
/************************************************************************/
int32_t determineNextEvent_relativeTime(tModbusEvent* next_modbus_event_p, const struct timespec *time_elapsed_p, struct timespec *max_timeout_p, struct suEventListHead *pEventListHead_p)
{
    int32_t mb_action_timeout = 0;
    struct schedulerEvent* pEvent = NULL;
    struct schedulerEvent* pNextEvent = NULL;

    struct timespec time_mb_tmp_action_timeout;
    time_mb_tmp_action_timeout.tv_sec = 0;
    time_mb_tmp_action_timeout.tv_nsec = 0;
    
    
    //store command with earliest due date
    pNextEvent = TAILQ_FIRST(pEventListHead_p);
    next_modbus_event_p->triggerTime = pNextEvent->triggerTime;
    next_modbus_event_p->ptModbusAction = pNextEvent->ptModbusAction;
    
    //remove entry with earliest due date from list
    TAILQ_REMOVE(pEventListHead_p, pNextEvent, events);	
    //add interval time to removed modbus action
    //pNextEvent->triggerTime.tv_sec = pNextEvent->triggerTime.tv_sec + pNextEvent->intervalTime.tv_sec;
    //pNextEvent->triggerTime.tv_nsec = pNextEvent->triggerTime.tv_nsec + pNextEvent->intervalTime.tv_nsec;
    timespec_add(&(pNextEvent->triggerTime), &(pNextEvent->triggerTime), &(pNextEvent->intervalTime));
    //insert entry according to the new due date
    insertEventByDueDate(pNextEvent, pEventListHead_p);


    //calculate new due dates for all actions
    TAILQ_FOREACH(pEvent, pEventListHead_p, events)
    {
        if ((pEvent->triggerTime.tv_sec == 0) 
            && (pEvent->triggerTime.tv_nsec == 0))
        {
            //next command has to be executed immediately
            //=> no time adjustment
        }
        else
        {
            timespec_diff(&(pEvent->triggerTime), &(pEvent->triggerTime), &(next_modbus_event_p->triggerTime));
            
            //further timing adjustment: subtract modbus processing time	
            if (timespec_diff(&(time_mb_tmp_action_timeout), &(pEvent->triggerTime), time_elapsed_p) < 0)
            {
                syslog(LOG_ERR, "modbus action backlog\n");
                mb_action_timeout = mb_action_timeout + 1;
                //timeout => set delay for action to zero and set new max timeout as return value if new delay is greater then max delay
                pEvent->triggerTime.tv_sec = 0;
                pEvent->triggerTime.tv_nsec = 0;
                struct timespec timespec_tmp;
                if (timespec_diff(&timespec_tmp, max_timeout_p, &time_mb_tmp_action_timeout) < 0)
                {
                    timespec_tmp.tv_sec = 0;
                    timespec_tmp.tv_nsec = 0;
                    timespec_diff(max_timeout_p, &timespec_tmp, &time_mb_tmp_action_timeout);
                    max_timeout_p->tv_sec = time_mb_tmp_action_timeout.tv_sec;
                    max_timeout_p->tv_nsec = time_mb_tmp_action_timeout.tv_nsec;
                }
            }
            else
            {
                pEvent->triggerTime.tv_sec = time_mb_tmp_action_timeout.tv_sec;
                pEvent->triggerTime.tv_nsec = time_mb_tmp_action_timeout.tv_nsec;
            }
        }
    }
    
#ifdef SCHEDULER_DEBUG
    syslog(LOG_INFO, "Update Modbus next actions list entry\n");
    TAILQ_FOREACH(pEvent, pEventListHead_p, events)
    {
        syslog(LOG_INFO, "Modbus next actions list entry: %d, %d, %d, %d s, %d msec, %d, %d, \n",
            pEvent->ptModbusAction->i8uSlaveAddress,
            pEvent->ptModbusAction->eFunctionCode,
            pEvent->ptModbusAction->i32uStartRegister,
            //pEvent->ptModbusAction->i8uCountRegister,
            (int)(pEvent->triggerTime.tv_sec),
            (int)(pEvent->triggerTime.tv_nsec/1000),
            pEvent->ptModbusAction->i32uStartByteProcessData,
            pEvent->ptModbusAction->i8uStartBitProcessData);
    }
#endif
    
    return mb_action_timeout;
}


/************************************************************************/
/** @ brief insert a scheduler event to the list according to its due date
 *  
 *  @param *newEvent_p scheduler event which has to be inserted
 */
/************************************************************************/
void insertEventByDueDate(struct schedulerEvent* newEvent_p, struct suEventListHead *pEventListHead_p)
{
    struct schedulerEvent* pEvent_l = NULL;
    bool bEventNotInserted_l = true;
    TAILQ_FOREACH(pEvent_l, pEventListHead_p, events)
    {
        if (newEvent_p->triggerTime.tv_sec < pEvent_l->triggerTime.tv_sec)
        {
            TAILQ_INSERT_BEFORE(pEvent_l, newEvent_p, events);
            bEventNotInserted_l = false;
            break;							
        }
        else if ((newEvent_p->triggerTime.tv_sec == pEvent_l->triggerTime.tv_sec)					
            && (newEvent_p->triggerTime.tv_nsec < pEvent_l->triggerTime.tv_nsec))
        {
            TAILQ_INSERT_BEFORE(pEvent_l, newEvent_p, events);
            bEventNotInserted_l = false;
            break;
        }				
    }
    //pNextEvent will be the last element of the list
    if (bEventNotInserted_l == true)
    {
        TAILQ_INSERT_TAIL(pEventListHead_p, newEvent_p, events);
    }
    
}



/************************************************************************/
/** @ brief get the minimal interval time of all events in the event list
 *  
 *  @param[out] struct timespec* min_interval_p minimal intervall time
 *	@param[in]	struct suEventListHead *pEventListHead_p head of scheduler event list
 *
 */
/************************************************************************/
void get_minimal_modbus_action_interval(struct timespec* min_interval_p, struct suEventListHead *pEventListHead_p)
{
    struct schedulerEvent* pEvent_l = NULL;
    //init with libmodbus standard timeout
    min_interval_p->tv_sec = INT32_MAX;
    min_interval_p->tv_nsec = s32_nanoseconds_per_second - 1;
    struct timespec tv_tmp_l;
    TAILQ_FOREACH(pEvent_l, pEventListHead_p, events)
    {
        if (timespec_diff(&tv_tmp_l, &(pEvent_l->intervalTime), min_interval_p) < 0)
        {
            min_interval_p->tv_sec = pEvent_l->intervalTime.tv_sec;
            min_interval_p->tv_nsec = pEvent_l->intervalTime.tv_nsec;
        }
    }
    return;
}


/*****************************************************************************/
/** @ brief get the minimal event time offset between two modbus actions
 *  
 *  @param[out] struct timespec* min_event_offset_p minimal event offset
 *	@param[in]	struct suEventListHead *pEventListHead_p head of scheduler event list
 *
 *  the minimal event time offset between two modbus actions should ensure
 *	the processing of the event list and should provide enough delay between
 *	the modbus commands to prevent overloading of slow modbus slaves
 */
/*****************************************************************************/
void get_minimal_modbus_event_offset(struct timespec* min_event_offset_p, struct suEventListHead *pEventListHead_p)
{
    //count number of events in list and allocate memory for the interval parameter
    struct schedulerEvent* pEvent_l = NULL;
    
    double inverse_mean_time_between_events = 0;
    double mean_time_between_events = 0;
    TAILQ_FOREACH(pEvent_l, pEventListHead_p, events)
    {
        uint64_t interval = pEvent_l->intervalTime.tv_nsec + pEvent_l->intervalTime.tv_sec*s32_nanoseconds_per_second;
        inverse_mean_time_between_events = inverse_mean_time_between_events + 1/((double)interval);
    }
    mean_time_between_events = 1 / inverse_mean_time_between_events;
    
    int64_t s64_mean_time_between_events = (int64_t)mean_time_between_events;
    //min_event_offset is a quarter of the mean time between events
    min_event_offset_p->tv_sec  = (int32_t)((s64_mean_time_between_events >> 2) / s32_nanoseconds_per_second);
    min_event_offset_p->tv_nsec = (int32_t)((s64_mean_time_between_events >> 2) % s32_nanoseconds_per_second);
    return;
}



/************************************************************************/
/** @ brief calculates the difference of two time values of type struct timespec
 *  
 *  @param *diff difference of the two time values *minuend and *subtrahend
 *	@param *minuend minuend of the calculation
 *	@param *subtrahend subtrahend of the calculation
 *  @return returns negative value if minuend < subtrahend, otherwise >= 0
 */
/************************************************************************/
int32_t timespec_diff(struct timespec *diff, const struct timespec *minuend, const struct timespec *subtrahend)
{

    assert(minuend->tv_nsec < s32_nanoseconds_per_second);
    assert(subtrahend->tv_nsec < s32_nanoseconds_per_second);
    
    if (subtrahend->tv_nsec > minuend->tv_nsec)
    {
        diff->tv_nsec = s32_nanoseconds_per_second - subtrahend->tv_nsec + minuend->tv_nsec;
        diff->tv_sec = minuend->tv_sec - subtrahend->tv_sec - 1;
    }
    else
    {
        diff->tv_nsec = minuend->tv_nsec - subtrahend->tv_nsec;
        diff->tv_sec = minuend->tv_sec - subtrahend->tv_sec;
    }	
    return diff->tv_sec;
}

/************************************************************************/
/** @ brief calculates the sum of two time values of type struct timespec
 *  
 *  @param[out] *sum sum of the two time values *summand1 and *summand2
 *	@param[in] *summand1
 *	@param[in] *summand2
 *  @return returns 0
 */
/************************************************************************/
int32_t timespec_add(struct timespec *sum, const struct timespec *summand1, const struct timespec *summand2)
{
    assert(summand1->tv_nsec < s32_nanoseconds_per_second);
    assert(summand2->tv_nsec < s32_nanoseconds_per_second);
    
    sum->tv_nsec = summand1->tv_nsec + summand2->tv_nsec;
    sum->tv_sec = summand1->tv_sec + summand2->tv_sec;
    if (sum->tv_nsec >= s32_nanoseconds_per_second)
    {
        //carry forward
        sum->tv_sec = sum->tv_sec + 1;
        sum->tv_nsec = sum->tv_nsec - s32_nanoseconds_per_second;
    }	
    return 0;
}