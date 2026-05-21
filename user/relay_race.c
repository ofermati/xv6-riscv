#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define TEAMS       3
#define RUNNERS     5
#define TARGET      30
#define FAVORITISM  100

int
race_over(void)
{
  for(int t = 0; t < TEAMS; t++)
    if(team_score_get(t) >= TARGET)
      return 1;
  return 0;
}

int
main(void)
{
  team_score_reset();
  int lock = israeli_create(FAVORITISM);

  for(int i = 0; i < TEAMS * RUNNERS; i++){
    int pid = fork();
    if(pid == 0){
      int team = i % TEAMS;
      setgid(team);

      while(!race_over()){
        israeli_acquire(lock);
        if(race_over()){
          israeli_release(lock);
          break;
        }
        int score = team_score_inc(team);
        printf("Runner %d (Team %d) acquired the baton\n", getpid(), team);
        printf("Team %d score = %d\n\n", team, score);
        israeli_release(lock);
        sleep(1);
      }
      exit(0);
    }
  }

  for(int i = 0; i < TEAMS * RUNNERS; i++)
    wait(0);

  israeli_destroy(lock);

  printf("Final scores:\n");
  for(int t = 0; t < TEAMS; t++)
    printf("  Team %d: %d\n", t, team_score_get(t));

  exit(0);
}
