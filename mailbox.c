#define mailbox_c

#include "mailbox.h"

#define NO_MAILBOXES 30

static void *shared_memory = NULL;
static mailbox *freelist = NULL;  /* list of free mailboxes.  */


/*
 *  initialise the data structures of mailbox.  Assign prev to the
 *  mailbox prev field.
 */

static mailbox *mailbox_config (mailbox *mbox, mailbox *prev)
{
  /* Initialise both the in and out values for the mailbox first. */
  mbox->in = 0;
  mbox->out = 0;
  /* Base code below. */
  mbox->data->result = 0;
  mbox->data->move_no = 0;
  mbox->data->positions_explored = 0;
  mbox->prev = prev;
  mbox->item_available = multiprocessor_initSem (0);
  mbox->space_available = multiprocessor_initSem (1);
  mbox->mutex = multiprocessor_initSem (1);
  return mbox;
}


/*
 *  init_memory - initialise the shared memory region once.
 *                It also initialises all mailboxes.
 */

static void init_memory (void)
{
  if (shared_memory == NULL)
    {
      mailbox *mbox;
      mailbox *prev = NULL;
      int i;
      _M2_multiprocessor_init ();
      shared_memory = multiprocessor_initSharedMemory
	(NO_MAILBOXES * sizeof (mailbox));
      mbox = shared_memory;
      for (i = 0; i < NO_MAILBOXES; i++)
	prev = mailbox_config (&mbox[i], prev);
      freelist = prev;
    }
}


/*
 *  init - create a single mailbox which can contain a single triple.
 */

mailbox *mailbox_init (void)
{
  mailbox *mbox;

  init_memory ();
  if (freelist == NULL)
    {
      printf ("exhausted mailboxes\n");
      exit (1);
    }
  mbox = freelist;
  freelist = freelist->prev;
  return mbox;
}


/*
 *  kill - return the mailbox to the freelist.  No process must use this
 *         mailbox.
 */

mailbox *mailbox_kill (mailbox *mbox)
{
  mbox->prev = freelist;
  freelist = mbox;
  return NULL;
}


/*
 *  send - send (result, move_no, positions_explored) to the mailbox mbox.
 */

void mailbox_send (mailbox *mbox, int result, int move_no, int positions_explored)
{
  /* Adds functionality to the mailbox to allow the sending of data. */
  multiprocessor_wait(mbox->space_available);                   // Waits for space to be available.
  multiprocessor_wait(mbox->mutex);                             // Waits for the mailbox to be accessible.
  /* Adds different points of data into the buffer. */
  mbox->data[mbox->in].result = result;                         // Result of the current go.
  mbox->data[mbox->in].move_no = move_no;                       // The move number of the current go.
  mbox->data[mbox->in].positions_explored = positions_explored; // The number of positions that have been explored in total.
  mbox->in = (mbox->in + 1) % MAX_MAILBOX_DATA;
  /* Adds the items into the mailbox buffer. */
  multiprocessor_signal(mbox->mutex);
  multiprocessor_signal(mbox->item_available);
}


/*
 *  rec - receive (result, move_no, positions_explored) from the
 *        mailbox mbox.
 */

void mailbox_rec (mailbox *mbox,
		  int *result, int *move_no, int *positions_explored)
{
  /* Adds functionality to the the the recieving of data and items in the mailbox. */
  multiprocessor_wait(mbox->item_available);
  multiprocessor_wait(mbox->mutex);
  /* Removes data and items from the buffer. */
  *result = mbox->data[mbox->out].result;                          // Pulls the results from mailbox.
  *move_no = mbox->data[mbox->out].move_no;                        // Pulls the move number from mailbox.
  *positions_explored = mbox->data[mbox->out].positions_explored;  // Pulls the positions explored from the mailbox.
  mbox->out = (mbox->out + 1) % MAX_MAILBOX_DATA;
  /* Updates items in the mailbox buffer. */
  multiprocessor_signal(mbox->mutex);
  multiprocessor_signal(mbox->space_available); 
}
