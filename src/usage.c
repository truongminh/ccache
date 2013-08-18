#include <getopt.h>
#include <limits.h>

char *program_name;
/* These enum values cannot possibly conflict with the option values
   ordinarily used by commands, including CHAR_MAX + 1, etc.  Avoid
   CHAR_MIN - 1, as it may equal -1, the getopt end-of-options value.  */

enum
{
  GETOPT_HELP_CHAR = (CHAR_MIN - 2),
  GETOPT_VERSION_CHAR = (CHAR_MIN - 3)
};

#define GETOPT_HELP_OPTION_DECL \
  "help", no_argument, NULL, GETOPT_HELP_CHAR
#define GETOPT_VERSION_OPTION_DECL \
  "version", no_argument, NULL, GETOPT_VERSION_CHAR
#define GETOPT_SELINUX_CONTEXT_OPTION_DECL \
  "context", required_argument, NULL, 'Z'

#define HELP_OPTION_DESCRIPTION \
  _("      --help     display this help and exit\n")
#define VERSION_OPTION_DESCRIPTION \
  _("      --version  output version information and exit\n")


static struct option const longopts[] =
{
  {GETOPT_SELINUX_CONTEXT_OPTION_DECL},
  {"port", required_argument, NULL, 'p'},
  {"src", required_argument, NULL, 's'},
  {"tmp", required_argument, NULL, 't'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};


struct ccache_options {
    char *addr;
    int port;
    char *srcd;
    char *tmpd;
};



void usage (int status)
{
  if (status != EXIT_SUCCESS)
    fprintf (stderr, ("Try `%s --help' for more information.\n"),program_name);
  else
    {
      printf (("Usage: %s [PORT]... [SRC_DIR]... [TMP_DIR]\n" \
              "With no SRC_DIR, the current directory is used as input dir.\n"\
              "With no TMP_DIR, the /tmp directory is used as tmp dir.\n"\
              "\n"),program_name);
    }

  exit (status);
}

struct ccache_options getOptions(int argc, char *argv[])
{
    struct ccache_options options;
    options.addr = "0.0.0.0";
    options.port = 80;
    options.srcd = ".";
    options.tmpd = ".";
    int optc;
    while ((optc = getopt_long (argc, argv, "ps:tZ:", longopts, NULL)) != -1)
      {
        switch (optc)
          {
          case 'p':
            if(!optarg || (options.port = atoi(optarg)) < 0) {
                printf("ERROR: Invalid port [%s].\nThe port must be a number greater than 0.\n",optarg);
                usage(EXIT_FAILURE);
            }
            break;
          case 's':
            options.srcd = optarg;
            break;
          case 't': /* --verbose  */
            options.tmpd = optarg;
            break;
          case GETOPT_HELP_CHAR:
            usage (EXIT_SUCCESS);
            break;
          case GETOPT_VERSION_CHAR:
            DISPLAY_DESCRIPTION;
            exit (EXIT_SUCCESS);
          default:
            usage (EXIT_FAILURE);
          }
      }
    return options;
}
