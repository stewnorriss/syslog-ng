#include "timerwheel.h"


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define NUM_TIMERS 10000

gint num_callbacks;
guint64 prev_now;

void
timer_callback(TimerWheel *self, guint64 now, gpointer user_data)
{
  guint64 expires = *(guint64 *) user_data;

  if (now != expires)
    {
      fprintf(stderr, "Expected time is not matching current time in callback, "
                      "now=%" G_GUINT64_FORMAT ", expires=%" G_GUINT64_FORMAT "\n",
                      now, expires);
      exit(1);
    }
  if (prev_now > now)
    {
      fprintf(stderr, "Callback current time is not monotonically increasing, "
                      "prev_now=%" G_GUINT64_FORMAT ", now=%" G_GUINT64_FORMAT "\n",
                      prev_now, now);
    }
  prev_now = now;
  num_callbacks++;
}

#define ASSOC_DATA_STRING "timerwheel associated data, check whether it's freed"

static void
_test_assoc_data(TimerWheel *wheel)
{
  timer_wheel_set_associated_data(wheel, g_strdup(ASSOC_DATA_STRING), (GDestroyNotify) g_free);
  if (strcmp(timer_wheel_get_associated_data(wheel), ASSOC_DATA_STRING) != 0)
    {
      fprintf(stderr, "Associated data mismatch, found=%s, expected=%s",
                      (gchar *) timer_wheel_get_associated_data(wheel), ASSOC_DATA_STRING);
      exit(1);
    }
}

void
test_wheel(gint seed)
{
  TimerWheel *wheel;
  gint i;
  guint64 latest = 0;
  gint expected_callbacks;

  prev_now = 0;
  num_callbacks = 0;
  expected_callbacks = 0;
  srand(seed);
  wheel = timer_wheel_new();
  _test_assoc_data(wheel);
  timer_wheel_set_time(wheel, 1);
  for (i = 0; i < NUM_TIMERS; i++)
    {
      guint64 expires;
      TWEntry *timer1, *timer2, *timer3;
      gint r;

      expires = rand() & ((1 << 24) - 1);
      if (expires <= 1)
        expires = 1;

      if (expires > latest)
        latest = expires;
      timer1 = timer_wheel_add_timer(wheel, expires - 1, timer_callback,
                                    g_memdup(&expires, sizeof(expires)), (GDestroyNotify) g_free);
      timer2 = timer_wheel_add_timer(wheel, expires - 1, timer_callback,
                                    g_memdup(&expires, sizeof(expires)), (GDestroyNotify) g_free);
      timer3 = timer_wheel_add_timer(wheel, expires - 1, timer_callback,
                                    g_memdup(&expires, sizeof(expires)), (GDestroyNotify) g_free);
      expected_callbacks += 3;
      r = rand() & 0xFF;
      if (r < 64)
        {
          /* delete the timer with 25% chance */
          timer_wheel_del_timer(wheel, timer1);
          expected_callbacks--;
        }
      else if (r < 128)
        {
          /* delete the timer with 25% chance */
          timer_wheel_del_timer(wheel, timer2);
          expected_callbacks--;
        }
      else if (r < 192)
        {
          /* delete the timer with 25% chance */
          timer_wheel_del_timer(wheel, timer3);
          expected_callbacks--;
        }
    }
  timer_wheel_set_time(wheel, latest + 1);
  if (num_callbacks != expected_callbacks)
    {
      fprintf(stderr, "Error: not enough callbacks received, "
                      "num_callbacks=%d, expected=%d\n",
                      num_callbacks, expected_callbacks);
      exit(1);
    }
  timer_wheel_free(wheel);
}

int
main()
{
  test_wheel(1234567890);
  test_wheel(time(NULL));
  return 0;
}
