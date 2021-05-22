#include <passmngr.h>
#include <kv_store.h>

#if defined(_MSC_VER)

    #include <conio.h>
    
    char getchar (void)
    {
        return (char) getch();
    }
#else
    #include <unistd.h>
    #include <termios.h>

    char getchar (void)
    {
        char buf = 0;
        struct termios oldt = {0};
        fflush(stdout);
        if(tcgetattr(0, &oldt) < 0)
            perror("tcsetattr()");
            
        oldt.c_lflag &= ~ICANON;
        oldt.c_lflag &= ~ECHO;
        oldt.c_cc[VMIN] = 1;
        oldt.c_cc[VTIME] = 0;
        if(tcsetattr(0, TCSANOW, &oldt) < 0)
            perror("tcsetattr ICANON");

        if(read(0, &buf, 1) < 0)
            perror("read()");

        oldt.c_lflag |= ICANON;
        oldt.c_lflag |= ECHO;
        if(tcsetattr(0, TCSADRAIN, &oldt) < 0)
            perror("tcsetattr ~ICANON");

        return buf;
    }
#endif

pm_empty_result_t input_password (byte_t (*password_buffer)[MAX_PASSWORD_SIZE], size_t* password_length)
{
    
}

int main (int argc, char** argv)
{
    
}