
#include <assert.h>
#include <unistd.h>

#include <utils.h>

int main() {
  if (getuid())
    pl_setup_user_ns();
  assert(getuid() == 0);
}
