#include <assert.h>
#include <plash.c>

#define TEST_FILE "/tmp/plashtest"

int main(){

        // disable buffering on stdout so we get all messages
        setvbuf(stdout, NULL, _IONBF, 0);
        char *from;
        char *to;

        puts("+ : simple subuid parse");
        pl_printf_to_file(TEST_FILE,
        "user1:42:2\n");
        from = to = NULL;
        assert(pl_parse_subid(TEST_FILE, "user1", "99", &from, &to) == 1);
        assert(strcmp(from, "42") == 0);
        assert(strcmp(to, "2") == 0);

        puts("+ : simple subuid parse find by query2");
        pl_printf_to_file(TEST_FILE,
        "99:42:2\n");
        from = to = NULL;
        assert(pl_parse_subid(TEST_FILE, "user", "99", &from, &to) == 1);
        assert(strcmp(from, "42") == 0);
        assert(strcmp(to, "2") == 0);

        puts("+ : subuid not found");
        pl_printf_to_file(TEST_FILE,
        "user1:42:2\n");
        from = to = NULL;
        assert(pl_parse_subid(TEST_FILE, "userXX", "99", &from, &to) == 0);

        puts("+ : multiple entries");
        from = to = NULL;
        pl_printf_to_file(TEST_FILE,
        "user2:100:10\nuser1:42:2\n");
        assert(pl_parse_subid(TEST_FILE, "user1", "99", &from, &to) == 1);
        assert(strcmp(from, "42") == 0);
        assert(strcmp(to, "2") == 0);

        puts("+ : no file");
        from = to = NULL;
        assert(pl_parse_subid("/doesnotexists", "user1", "99", &from, &to) == 0);

        puts("+ : empty file");
        pl_printf_to_file(TEST_FILE, "");
        from = to = NULL;
        assert(pl_parse_subid(TEST_FILE, "user1", "99", &from, &to) == 0);

        puts("+ : bad file");
        pl_printf_to_file(TEST_FILE, "XXXX");
        from = to = NULL;
        assert(pl_parse_subid(TEST_FILE, "user1", "99", &from, &to) == 0);

        puts("+ : another bad file");
        pl_printf_to_file(TEST_FILE, "XXXX:XXXX\n");
        from = to = NULL;
        assert(pl_parse_subid(TEST_FILE, "user1", "99", &from, &to) == 0);

        puts("+ : multiple entries => take the first one");
        pl_printf_to_file(TEST_FILE,
        "user1:1:2\nuser1:3:4");
        from = to = NULL;
        assert(pl_parse_subid(TEST_FILE, "user1", "99", &from, &to) == 1);
        assert(strcmp(from, "1") == 0);
        assert(strcmp(to, "2") == 0);

        puts("+ : no newline in end of file");
        pl_printf_to_file(TEST_FILE,
        "user1:1:2");
        from = to = NULL;
        assert(pl_parse_subid(TEST_FILE, "user1", "99", &from, &to) == 1);
        assert(strcmp(from, "1") == 0);
        assert(strcmp(to, "2") == 0);

        puts("+ : no newline in end of file, big ids");
        pl_printf_to_file(TEST_FILE,
        "user1:1000:2000");
        from = to = NULL;
        assert(pl_parse_subid(TEST_FILE, "user1", "99", &from, &to) == 1);
        assert(strcmp(from, "1000") == 0);
        assert(strcmp(to, "2000") == 0);

        puts("+ : no newline, multiple entries, first match");
        pl_printf_to_file(TEST_FILE,
        "user1:1:2\nuser2:3:4");
        from = to = NULL;
        assert(pl_parse_subid(TEST_FILE, "user1", "99", &from, &to) == 1);
        assert(strcmp(from, "1") == 0);
        assert(strcmp(to, "2") == 0);

        puts("+ : no newline, multiple entries, last match");
        pl_printf_to_file(TEST_FILE,
        "user1:1:2\nuser2:3:4");
        from = to = NULL;
        assert(pl_parse_subid(TEST_FILE, "user2", "99", &from, &to) == 1);
        assert(strcmp(from, "3") == 0);
        assert(strcmp(to, "4") == 0);

}
