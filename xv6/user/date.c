#include "types.h"
#include "user.h"
#include "date.h"

int main(int argc, char *argv[])
{
  struct rtcdate r;

  if(date(&r) < 0){
    printf(2, "date: failed to get date\n");
    exit(0);
  }
  printf(1, "Current date: %d/%d/%d %d:%d:%d\n", r.year, r.month, r.day, r.hour, r.minute, r.second);
  exit(0);
}