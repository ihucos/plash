
#include <assert.h>
#include <unistd.h>

#include <plash.h>

int main(){
        pl_unshare();
        assert(getuid() == 0);
}
