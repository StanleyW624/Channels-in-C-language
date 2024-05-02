#include "channel.h"

// Creates a new channel with the provided size and returns it to the caller
// A 0 size indicates an unbuffered channel, whereas a positive size indicates a buffered channel
chan_t* channel_create(size_t size)
{
    /* IMPLEMENT THIS */

    // Initizalize buffer
    chan_t* channel = (chan_t*) malloc(sizeof(chan_t));

    // If fails to create return NULL
    if (channel == NULL) {
        return NULL;
    }

    // Check given size, buffered channel has a size > 0, else it is unbuffered
    if (size > 0) {
        channel -> buffer = buffer_create(size); //buffered
    } else{
        channel -> buffer = NULL; //unbuffered
    }
        
    // Initialize all mutexes
    pthread_mutex_init(&channel -> mutex, NULL);
    pthread_mutex_init(&channel -> Smutex, NULL);
    pthread_mutex_init(&channel -> Rmutex, NULL);

    // Initialize all semaphore    
    sem_init(&channel -> sem_send, 0, (unsigned int) size);
    sem_init(&channel -> sem_recv, 0, 0);

    // Create list for each 
    channel->send_list = list_create();
    channel->recv_list = list_create();
    
    // Set default close status to false
    channel->close_status = false;

    return channel;
}

// Writes data to the given channel
// This can be both a blocking call i.e., the function only returns on a successful completion of send (blocking = true), and
// a non-blocking call i.e., the function simply returns if the channel is full (blocking = false)
// In case of the blocking call when the channel is full, the function waits till the channel has space to write the new data
// Returns SUCCESS for successfully writing data to the channel,
// WOULDBLOCK if the channel is full and the data was not added to the buffer (non-blocking calls only),
// CLOSED_ERROR if the channel is closed, and
// OTHER_ERROR on encountering any other generic error of any sort
enum chan_status channel_send(chan_t* channel, void* data, bool blocking)
{
    /* IMPLEMENT THIS */
    
    // Indicator for close status
    bool r_close_status;

    if (channel == NULL) {
        return OTHER_ERROR;
    }

    // Check for blocking
    if (blocking == false) {
        
        // If not blocked, trywait
        if (sem_trywait(&channel->sem_send) != 0) {

            // If trywait fails returns status
            pthread_mutex_lock(&channel->mutex);
            r_close_status = channel->close_status;
            pthread_mutex_unlock(&channel->mutex);
            return r_close_status ? CLOSED_ERROR : WOULDBLOCK;
        }

    } else {
        sem_wait(&channel->sem_send);
    }

    pthread_mutex_lock(&channel->mutex);

    // Check close status
    if ((channel -> close_status) == true) {

        pthread_mutex_unlock(&channel->mutex);;
        sem_post(&channel->sem_send);

        return CLOSED_ERROR;
    }

    // Add data to buffer
    buffer_add(data,channel->buffer);

    pthread_mutex_unlock(&channel->mutex);

    sem_post(&channel->sem_recv);

    pthread_mutex_lock(&channel->Rmutex);
    
    // Wake up all reciever
    if (list_count(channel->recv_list) > 0) {
        list_foreach(channel->recv_list,(void*) sem_post);
        pthread_mutex_unlock(&channel->Rmutex);
    } else {
        pthread_mutex_unlock(&channel->Rmutex);
    }

    return SUCCESS;
}

// Reads data from the given channel and stores it in the function’s input parameter, data (Note that it is a double pointer).
// This can be both a blocking call i.e., the function only returns on a successful completion of receive (blocking = true), and
// a non-blocking call i.e., the function simply returns if the channel is empty (blocking = false)
// In case of the blocking call when the channel is empty, the function waits till the channel has some data to read
// Returns SUCCESS for successful retrieval of data,
// WOULDBLOCK if the channel is empty and nothing was stored in data (non-blocking calls only),
// CLOSED_ERROR if the channel is closed, and
// OTHER_ERROR on encountering any other generic error of any sort
enum chan_status channel_receive(chan_t* channel, void** data, bool blocking)
{
    /* IMPLEMENT THIS */

    // Indicator for close status
    bool r_close_status;

    if (channel == NULL) {
        return OTHER_ERROR;
    }

    // Check for blocking
    if (blocking == false) {
        
        // If not blocked, trywait
        if (sem_trywait(&channel->sem_recv) != 0) {

            // If trywait fails returns status
            pthread_mutex_lock(&channel->mutex);
            r_close_status = channel->close_status;
            pthread_mutex_unlock(&channel->mutex);
            return r_close_status ? CLOSED_ERROR : WOULDBLOCK;
        }

    } else {
        sem_wait(&channel->sem_recv);
    }

    pthread_mutex_lock(&channel->mutex);

    // Check close status
    if ((channel -> close_status) == true) {

        pthread_mutex_unlock(&channel->mutex);
        sem_post(&channel->sem_recv);
        
        pthread_mutex_lock(&channel->Rmutex);
    
        // Wake up all reciever
        if (list_count(channel->recv_list) > 0) {
            list_foreach(channel->recv_list, (void*) sem_post);
            pthread_mutex_unlock(&channel->Rmutex);
        } else {;
            pthread_mutex_unlock(&channel->Rmutex);
        }

        return CLOSED_ERROR;
    }

    // Remove from buffer
    *data = buffer_remove(channel->buffer);

    pthread_mutex_unlock(&channel->mutex);

    sem_post(&channel->sem_send);

    pthread_mutex_lock(&channel->Smutex);
    
    // Wake up all sender
    if (list_count(channel->send_list) > 0) {
        list_foreach(channel->send_list, (void*) sem_post);
        pthread_mutex_unlock(&channel->Smutex);
    } else {
        pthread_mutex_unlock(&channel->Smutex);
    }
    
    return SUCCESS;
}

// Closes the channel and informs all the blocking send/receive/select calls to return with CLOSED_ERROR
// Once the channel is closed, send/receive/select operations will cease to function and just return CLOSED_ERROR
// Returns SUCCESS if close is successful,
// CLOSED_ERROR if the channel is already closed, and
// OTHER_ERROR in any other error case
enum chan_status channel_close(chan_t* channel)
{
    /* IMPLEMENT THIS */

    if (channel == NULL) {
        return OTHER_ERROR;
    }

    pthread_mutex_lock(&channel->mutex);

    // Check for close status
    if ((channel -> close_status) == true) {
        pthread_mutex_unlock(&channel->mutex);
        return CLOSED_ERROR;
    }

    // Set close status to true
    channel -> close_status = true;

    pthread_mutex_unlock(&channel->mutex);
    
    sem_post(&channel->sem_send);
    sem_post(&channel->sem_recv);

    pthread_mutex_lock(&channel->Smutex);

    // Wakup all sender
    if (list_count(channel->send_list) > 0) {
        list_foreach(channel->send_list, (void*) sem_post);
        pthread_mutex_unlock(&channel->Smutex);
    } else {
        pthread_mutex_unlock(&channel->Smutex);
    }

    pthread_mutex_lock(&channel->Rmutex);
    
    // Wakup all reciever
    if (list_count(channel->recv_list) > 0) {
        list_foreach(channel->recv_list,(void*) sem_post);
        pthread_mutex_unlock(&channel->Rmutex);
    } else {
        pthread_mutex_unlock(&channel->Rmutex);
    }

    return SUCCESS;
}

// Frees all the memory allocated to the channel
// The caller is responsible for calling channel_close and waiting for all threads to finish their tasks before calling channel_destroy
// Returns SUCCESS if destroy is successful,
// DESTROY_ERROR if channel_destroy is called on an open channel, and
// OTHER_ERROR in any other error case
enum chan_status channel_destroy(chan_t* channel)
{
    /* IMPLEMENT THIS */
    
    if (channel == NULL) {
        return OTHER_ERROR;
    }

    // Check to see if channels are closed
    if ((channel -> close_status) == false) {
        return DESTROY_ERROR;
    }

    // Free up buffer
    buffer_free(channel->buffer);

    // Destroy all semaphore
    sem_destroy(&channel->sem_send);
    sem_destroy(&channel->sem_recv);

    // Free up all list
    list_destroy(channel->recv_list);
    list_destroy(channel->send_list);

    // Destroy all mutex
    pthread_mutex_destroy(&channel->mutex);
    pthread_mutex_destroy(&channel->Smutex);
    pthread_mutex_destroy(&channel->Rmutex);

    // Free up channel
    free(channel);
    
    return SUCCESS;
}

// Takes an array of channels, channel_list, of type select_t and the array length, channel_count, as inputs
// This API iterates over the provided list and finds the set of possible channels which can be used to invoke the required operation (send or receive) specified in select_t
// If multiple options are available, it selects the first option and performs its corresponding action
// If no channel is available, the call is blocked and waits till it finds a channel which supports its required operation
// Once an operation has been successfully performed, select should set selected_index to the index of the channel that performed the operation and then return SUCCESS
// In the event that a channel is closed or encounters any error, the error should be propagated and returned through select
// Additionally, selected_index is set to the index of the channel that generated the error
enum chan_status channel_select(size_t channel_count, select_t* channel_list, size_t* selected_index)
{
    /* IMPLEMENT THIS */

    enum chan_status status = OTHER_ERROR;

    // Runs the first cycle, return status if does not need to wait
    for (size_t i = 0; i< channel_count; i++) {
        if ((channel_list[i].is_send) == false) {
            status = channel_receive(channel_list[i].channel, &channel_list[i].data, false); // for receive
        } else {
            status = channel_send(channel_list[i].channel, channel_list[i].data, false); // for send
        }

        // If returns, return
        if (status != WOULDBLOCK) {
            *selected_index = (size_t) i;
            return status;
        } 
    }
    
    // Initalize local semaphore
    sem_t sem_sel;
    sem_init(&sem_sel, 0, 1);

    // Inseart status of channel into list
    for (size_t i = 0; i< channel_count; i++) {
        if ((channel_list[i].is_send) == false) {

            pthread_mutex_lock(&(channel_list[i].channel)->Rmutex);

            // If already in list, then dont insert
            if ((list_find((&channel_list[i])->channel->recv_list, &sem_sel)) == NULL) {
                list_insert((&channel_list[i])->channel->recv_list, &sem_sel);
            }

            pthread_mutex_unlock(&(channel_list[i].channel)->Rmutex);

        } else {


            pthread_mutex_lock(&(channel_list[i].channel)->Smutex);

            // If already in list, then dont insert
            if ((list_find((&channel_list[i])->channel->send_list, &sem_sel)) == NULL) {
                list_insert((&channel_list[i])->channel->send_list, &sem_sel);
            }

            pthread_mutex_unlock(&(channel_list[i].channel)->Smutex);

        }

    }

    // Keep track of return status
    bool complete = false;

    while (1) {
        
        // Retest all the channel
        for (size_t i = 0; i< channel_count; i++) {
            if ((channel_list[i].is_send) == false) {
                status = channel_receive(channel_list[i].channel, &channel_list[i].data, false); // for receive

                // If returns, return
                if (status != WOULDBLOCK) {
                    *selected_index = (size_t) i;
                    complete = true;
                    break;
                }
            } else {
                status = channel_send(channel_list[i].channel, channel_list[i].data, false); // for send

                // If returns, return
                if (status != WOULDBLOCK) {
                    *selected_index = (size_t) i;
                    complete = true;
                    break;
                }
            }
        }
        

        // Check for sem wait
        if (complete == true) {
            break;
        } else {
            sem_wait(&sem_sel);
        }
        
    }

    // Clean up list
    for (size_t i = 0; i<channel_count; i++) {
        if ((channel_list[i].is_send) == false) {

            pthread_mutex_lock(&(channel_list[i].channel)->Rmutex);

            // If found within the list, remove from list
            if (list_find(channel_list[i].channel->recv_list, &sem_sel) != NULL) {
                list_remove(channel_list[i].channel->recv_list, list_find(channel_list[i].channel->recv_list, &sem_sel));
            }
            pthread_mutex_unlock(&(channel_list[i].channel)->Rmutex);
        } else {
            pthread_mutex_lock(&(channel_list[i].channel)->Smutex);

            // If found within the list, remove from list
            if ((list_find(channel_list[i].channel->send_list, &sem_sel))!= NULL) {
                list_remove(channel_list[i].channel->send_list, list_find(channel_list[i].channel->send_list, &sem_sel));
            }
            pthread_mutex_unlock(&(channel_list[i].channel)->Smutex);
        }
    }

    // Destroy local semaphore
    sem_destroy(&sem_sel);
    return status;
}
