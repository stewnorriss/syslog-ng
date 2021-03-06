#include "date-parser.h"
#include "apphook.h"
#include "testutils.h"
#include "template_lib.h"

#include <locale.h>
#include <stdlib.h>

static void
testcase(gchar *msg, gchar *timezone, gchar *format, gchar *expected)
{
  LogTemplate *templ;
  LogMessage *logmsg;
  LogParser *parser;
  gboolean success;
  GString *res = g_string_sized_new(128);

  parser = date_parser_new (configuration);
  if (format != NULL) date_parser_set_format(parser, format);
  if (timezone != NULL) date_parser_set_timezone(parser, timezone);

  log_pipe_init(&parser->super);

  logmsg = log_msg_new_empty();
  logmsg->timestamps[LM_TS_RECVD].tv_sec = 1451473200; /* Dec  30 2015 */
  log_msg_set_value(logmsg, log_msg_get_value_handle("MESSAGE"), msg, -1);
  success = log_parser_process(parser, &logmsg, NULL, log_msg_get_value(logmsg, LM_V_MESSAGE, NULL), -1);

  if (!success && expected)
    {
      fprintf(stderr, "unable to parse format=%s msg=%s\n", format, msg);
      exit(1);
    }
  else if (success && !expected)
    {
      fprintf(stderr, "successfully parsed but expected failure, format=%s msg=%s\n", format, msg);
      exit(1);
    }
  else if (expected)
    {
      /* Convert to ISODATE */
      templ = compile_template("${ISODATE}", FALSE);
      log_template_format(templ, logmsg, NULL, LTZ_LOCAL, -1, NULL, res);
      assert_nstring(res->str, res->len, expected, strlen(expected),
                     "incorrect date parsed msg=%s format=%s",
                     msg, format);
      log_template_unref(templ);
    }

  g_string_free(res, TRUE);
  log_pipe_unref(&parser->super);
  log_msg_unref(logmsg);
  return;
}


int main()
{
  app_startup();

  setlocale (LC_ALL, "C");
  putenv("TZ=CET-1");
  tzset();

  configuration = cfg_new(0x0302);

  /* Various ISO8601 formats */
  testcase("2015-01-26T16:14:49+0300", NULL, NULL, "2015-01-26T16:14:49+03:00");
  testcase("2015-01-26T16:14:49+0330", NULL, NULL, "2015-01-26T16:14:49+03:30");
  testcase("2015-01-26T16:14:49+0200", NULL, NULL, "2015-01-26T16:14:49+02:00");
  testcase("2015-01-26T16:14:49+03:00", NULL, NULL, "2015-01-26T16:14:49+03:00");
  testcase("2015-01-26T16:14:49+03:30", NULL, NULL, "2015-01-26T16:14:49+03:30");
  testcase("2015-01-26T16:14:49+02:00", NULL, NULL, "2015-01-26T16:14:49+02:00");
  testcase("2015-01-26T16:14:49Z", NULL, NULL, "2015-01-26T16:14:49+00:00");
  testcase("2015-01-26T16:14:49A", NULL, NULL, "2015-01-26T16:14:49-01:00");
  testcase("2015-01-26T16:14:49B", NULL, NULL, "2015-01-26T16:14:49-02:00");
  testcase("2015-01-26T16:14:49N", NULL, NULL, "2015-01-26T16:14:49+01:00");
  testcase("2015-01-26T16:14:49O", NULL, NULL, "2015-01-26T16:14:49+02:00");
  testcase("2015-01-26T16:14:49GMT", NULL, NULL, "2015-01-26T16:14:49+00:00");
  testcase("2015-01-26T16:14:49PDT", NULL, NULL, "2015-01-26T16:14:49-07:00");

  /* RFC 2822 */
  testcase("Tue, 27 Jan 2015 11:48:46 +0200", NULL, "%a, %d %b %Y %T %z", "2015-01-27T11:48:46+02:00");

  /* Apache-like */
  testcase("21/Jan/2015:14:40:07 +0500", NULL, "%d/%b/%Y:%T %z", "2015-01-21T14:40:07+05:00");

  /* Try with additional text at the end, should fail */
  testcase("2015-01-26T16:14:49+0300 Disappointing log file", NULL, NULL, NULL);

  /* Dates without timezones. America/Phoenix has no DST */
  testcase("Tue, 27 Jan 2015 11:48:46", NULL, "%a, %d %b %Y %T", "2015-01-27T11:48:46+01:00");
  testcase("Tue, 27 Jan 2015 11:48:46", "America/Phoenix", "%a, %d %b %Y %T", "2015-01-27T11:48:46-07:00");
  testcase("Tue, 27 Jan 2015 11:48:46", "+05:00", "%a, %d %b %Y %T", "2015-01-27T11:48:46+05:00");

  /* Try without the year. */
  testcase("01/Jan:00:40:07 +0500", NULL, "%d/%b:%T %z", "2016-01-01T00:40:07+05:00");
  testcase("01/Aug:00:40:07 +0500", NULL, "%d/%b:%T %z", "2015-08-01T00:40:07+05:00");
  testcase("01/Sep:00:40:07 +0500", NULL, "%d/%b:%T %z", "2015-09-01T00:40:07+05:00");
  testcase("01/Oct:00:40:07 +0500", NULL, "%d/%b:%T %z", "2015-10-01T00:40:07+05:00");
  testcase("01/Nov:00:40:07 +0500", NULL, "%d/%b:%T %z", "2015-11-01T00:40:07+05:00");


  testcase("1446128356 +01:00", NULL, "%s %z", "2015-10-29T15:19:16+01:00");
  testcase("1446128356", "Europe/Budapest", "%s", "2015-10-29T15:19:16+01:00");

  app_shutdown();
  return 0;
};
