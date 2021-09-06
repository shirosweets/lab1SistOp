#include <assert.h>
#include <fcntl.h> // open
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "builtin.h"
#include "command.h"
#include "execute.h"


/* Pone, si existe, el archivo de redireción de entrada en el stdin,
 * si no existe no hace nada
 * Returns: 0 si la operación fue exitosa
 *          1 si la operación falló
 *
 * Requires: cmd != NULL
 *
 */
static int change_file_descriptor_in(scommand cmd) {
    assert(cmd != NULL);

    char* redir_in = scommand_get_redir_in(cmd);
    // si la redirección de entrada no está seeteada, scommand_get_redir_in devuelve NULL
    if (redir_in != NULL) {
        int file_redir_in = open(redir_in, O_RDONLY);
        // Si el archivo no existe, o hay algún otro tipo de error, open retorna
        // -1, si no retorna el descriptor del archivo
        if (file_redir_in == -1) {
            // En caso de error, open seetea el mansaje de perror
            perror(redir_in);
            return (-1);
        }

        int dup2_res = dup2(file_redir_in, STDIN_FILENO);
        // Si dup2 falla, retorna -1
        if (dup2_res == -1) {
            // dup2 no suele fallar, pero podría llegar a hacerlo
            // Si lo hace, dup2 seetea el mensaje de perror
            perror("dup2");
            return (-1);
        }

        int res_close = close(file_redir_in);
        if (res_close == -1) {
            // No debería fallar, PERO si falla...
            perror("dup2");
            return (-1);
        }
    }
    return (0);
}

/* Pone, si existe, el archivo de redirección de salida en el stdout,
 * si el archivo no existe lo crea.
 * Si la redirección de salida no está seeteada no hace nada
 * Returns: 0 si la operación fue exitosa
 *          1 si la operación falló
 *
 * Requires: cmd != NULL
 *
 */
static int change_file_descriptor_out(scommand cmd) {
    assert(cmd != NULL);

    char* redir_out = scommand_get_redir_out(cmd);
    // si la redirección de salida no está seeteada, scommand_get_redir_out devuelve NULL
    if (redir_out != NULL) {

        /*  open como segundo parametro toma flags, en este caso tiene los
           flags O_WRONLY que hace que el archivo se abra en solo escritura, y
           O_CREAT que hace que si el archivo no existe, se cree. En el caso de
           estar seeteado O_CREAT, open toma un tercer parametro de flags de
           cración del archivo, en este caso están puestos los flags:
               S_IRUSR: user has read permission
               S_IWUSR: user has write permission
             */

        int file_redir_out =
            open(redir_out, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);

        // Si hay algún error devuelve -1, en caso de que no exista el archivo
        // lo crea, si no hay error retorna el descriptor del archivo
        if (file_redir_out == -1) {
            // En caso de error, open seetea el mansaje de perror
            perror(redir_out);
            return (-1);
        }

        int dup2_res = dup2(file_redir_out, STDOUT_FILENO);
        // Si dup2 falla, retorna -1
        if (dup2_res == -1) {
            // dup2 no suele fallar, pero podría llegar a hacerlo
            // Si lo hace, dup2 seetea el mensaje de perror
            perror("dup2");
            return (-1);
        }

        int res_close = close(file_redir_out);
        if (res_close == -1) {
            // No debería fallar, PERO si falla...
            perror("close");
            return (-1);
        }
    }
    return (0);
}

/* Ejecuta un comando como comando externo en el mismo proceso, es decir sin hacer
 * fork, redirigiendo el stdin y el stdout si están seeteados
 * Puede modificar cmd, pero no destruirlo
 * Si la llamada sale bien no se retorna, si la llamada sale mal, si hay algún
 * problema con la redirección de entrada o de salida se retorna -1, y si la llamada
 * al programa falla se retorna el código de error
 * En caso de fallar la llamada al programa, los descriptores de archivo quedan cambiados
 *
 * Requires: cmd != NULL && !scommand_is_empty(cmd)
 *
 */
static int scommand_exec_external(scommand cmd) {
    assert(cmd != NULL && !scommand_is_empty(cmd));

    // Se cambia stdin por el archivo de redirección de entrada, si es que está seeteado
    int exit_redir_in = change_file_descriptor_in(cmd);
    if (exit_redir_in != 0) {
        return (-1);
    }

    // Se cambia stdout por el archivo de redirección de salida, si es que está seeteado
    int exit_redir_out = change_file_descriptor_out(cmd);
    if (exit_redir_out != 0) {
        return (-1);
    }

    char** argv = scommand_to_argv(cmd);
    // argv != NULL  por poscondición de scommand_to_argv
    int ret_code = execvp(argv[0], argv);

    /* Si execvp falla (y por ende retorna) se imprime un mensaje
      y se libera la memoria */
    perror(argv[0]);
    ret_code = ret_code;

    unsigned int j = 0u;
    while (argv[j] != NULL) {
        free(argv[j]);
        argv[j] = NULL;
        j++;
    }
    free(argv);
    argv = NULL;

    return ret_code;
}

/* Ejecuta un comando, tanto si es interno como si es externo
 * En el caso de ser externo solo retorna si hay un error, y en ese caso,
 * devuelve -1. En caso se ser un interno, retorna 0.
 * Puede modificar cmd, pero no destruirlo
 * 
 * En caso de fallar la llamada al programa, los descriptores de archivo pueden quedar
 * cambiados por los de redirección de entrada y de salida
 *
 * Requires: cmd != NULL
 */
static int scommand_exec(scommand cmd) {
    assert(cmd != NULL);
    int ret_code = 0;
    if (builtin_scommand_is_internal(cmd)) {
        builtin_scommand_exec(cmd);
        ret_code = 0;
    } else if (!scommand_is_empty(cmd)) {
        scommand_exec_external(cmd);
    }

    return (ret_code);
}

/* Ejecuta un pipeline de un solo comando tanto si es interno como si es externo
 * en caso de ser externo hace fork pero en caso de ser interno no
 *
 * Requires: apipe != NULL && pipeline_length(apipe) == 1
 */
static void single_command(pipeline apipe) {
    assert(apipe != NULL && pipeline_length(apipe) == 1);

    if (builtin_scommand_is_single_internal(apipe)) {
        // Caso en el que comando es interno 
        builtin_single_pipeline_exec(apipe);
    } else {
        //Caso en el que el comando es externo y se debe hacer fork()
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            return;
        } else if (pid == 0) {
            scommand_exec(pipeline_front(apipe));
            _exit(1);
        }
        // El proceso padre solo espera a los hijos en caso de que no se indique el caracter
        // & en el pipeline (osea, cuando está seeteado para que espere)
        if (pipeline_get_wait(apipe)) {
            waitpid(pid, NULL, 0);
        } 
    }
}

/* Como dup2, solo que ademas (si oldfd == newfd no hace nada y retorna newfd)
 * cierra oldfd y en caso de fallo, imprime un mensaje de error
 */
static int dup2_extra(int oldfd, int newfd) {
    if (oldfd != newfd) {
        newfd = dup2(oldfd, newfd);
        if (newfd < 0) {
            // En caso de que dup2 falle se hace que perror imprima el mensaje de error
            perror("dup2:");
        }
        close(oldfd);
    }

    return (newfd);
}

/* Ejecutá un pipeline de multiples comandos (2 o mas) haciendo fork para cada comando
 * (incluso para los internos) y si está seeteado para que espere, espera a que terminen
 * 
 * Puede modificar apipe pero no destruirlo, en caso de que no haya ningún error,
 * deja vacio a apipe
 *
 * Requires: apipe != NULL && pipeline_length(apipe) >= 2
 * 
 * Ensures: apipe != NULL
 */
static void multiple_commands(pipeline apipe) {
    assert(apipe != NULL && pipeline_length(apipe) >= 2);

    int pipefd[2];
    int fd_in = STDIN_FILENO;
    int child_processes_running = 0;
    
    bool error_flag = false;
    // Variable para volver true si hay un error

    // Caso en el que haya un pipeline multiple 
    while (!error_flag && !pipeline_is_empty(apipe)) {
        int res_pipe = pipe(pipefd);
        if (res_pipe < 0) {
            perror("pipe");
            error_flag = true;
        }
        else {
            pid_t pid = fork();
            if (pid  == -1) {
                perror("fork");
                error_flag = true;
            }
            else if (pid == 0) {

                int res_dup = dup2_extra(fd_in, STDIN_FILENO);
                if(res_dup < 0){
                    _exit(1);
                }  

                // Si el comando no es el ultimo se coloca la salida del pipe
                // en el stdout
                if(pipeline_length(apipe) > 1) {
                    res_dup = dup2(pipefd[1], STDOUT_FILENO);
                    if (res_dup < 0) {
                        perror("dup2");
                        _exit(1);
                    }
                }

                close(pipefd[0]);
                close(pipefd[1]);
                scommand_exec(pipeline_front(apipe));
                /* En caso de el el comando haya sido interno, o haya habido un fallo,
                   se termina la ejecución del hijo */
                _exit(1);
            }
            child_processes_running++;
            
            if (child_processes_running > 1) {
                // Esto se ejecuta siempre salvo en el primer siclo del while 
                /* En el primer siclo de while no hay que cerrar fd_in porque 
                   es el stdin, en el resto es la punta de lectura del pipe */
                close(fd_in);
            }

            close(pipefd[1]);
            fd_in = pipefd[0];
            pipeline_pop_front(apipe);
        }
    }
    close(pipefd[0]);

    // El proceso padre solo va a esperar en caso de que no se encuentre el caracter
    // & en el pipeline
    if (pipeline_get_wait(apipe)) {
        while(child_processes_running > 0) {
            wait(NULL);
            child_processes_running--;
        }
    } 
}


/* Se encarga de ejecutar todos los comandos que se ingresan en el bash, desde los 
 * comandos simples (externos e internos), como los pipelines de 2 o más comandos, en el bash
 * se puede pasar el parámetro & el cual indica que los procesos se corren en background, es
 * decir el proceso padre no espera a que terminen los mismos.
 * No devuelve nada, dentro de los procesos hijos se realiza el manejo de los errores para
 * que la función muestre un mensaje de error en caso de que un dup2, fork o pipe falle.
 * No destruye el pipeline, pero si elimina sus elementos con pipeline_front_pop()
*/
void execute_pipeline(pipeline p){
    int numberOfPipes = pipeline_length(p) - 1;
    //Caso en el que el pipe solo tiene un comando
    zombie_handler();
    if (numberOfPipes == 0) {
        single_command(p);
    } else {
        multiple_commands(p);
    }
}

void zombie_handler(){
    pid_t pid;
    for (pid = waitpid(-1 ,NULL,WNOHANG);
             pid != 0 && pid != -1;
             pid = waitpid(-1,NULL,WNOHANG)){
                 wait(NULL);
    }
}
