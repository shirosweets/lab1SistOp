- [x] Transcribir lo importante de la consigna a [consigna.md](consigna.md)
- **Command**:
    - [x] Terminar `scommand_to_string` en [command.c](skeleton2021/command.c)
    - [x] Verificar que el TAD de command no tenga errores
    - [x] Verificar los comandos añadidos de las pipelines
    - [x] Implementar `pipeline_pop_front` en [command.c](skeleton2021/command.c)
    - [x] Los `pop_front` tanto de `pipeline` como de `scommand` andan mal cuando queda un solo elemento
    - [x] Se verifica que `scommand_pop_front` funcione correctamente, se cambia la implementación de la función

- **builtin**:
    - [x] Agregar soporte para usar `'~'` para referirse al home en `builtin` (teniendo en cuenta el caso de que haya una carpeta cuyo nombre empiece con `~`).
    En bash en el caso de que hay una carpeta cuyo nombre empieza con `~` y tiene mas letras, se puede entrar a la carpeta con `cd ~nombreDeLaCarpeta` o con `cd \~nombreDeLaCarpeta` y en caso de que la carpeta se llame `~` solo, se puede entrar con `cd \~`
    - [x] Explicar `change_file_descriptor_in` y `change_file_descriptor_out`.

- **execute**
    - [x] Modificar la función execute_pipeline para que pueda ejecutar cualquier pipeline independientemente de la cantidad de comandos que tenga, ademas que no tenga problemas en ejecutarse si se combinan comandos internos con comandos externos, y que permita ejecutar los pipelines en background con el carácter &.
    - [x] Modificar la función execute_pipeline para que no queden procesos zombies al correrse pipelines en modo background.
    - [x] Modificar la funcion execute_pipeline para que no ocurra una funcion de input si se manda la misma al background, por ejemplo `wc &`.

- **prompt**:
    - [x] Realizar el punto estrella.

- Agregar comprensión en [informe.md](informe.md) de la parte de:
    - [x] `builtin` [builtin.c](skeleton2021/builtin.c)
    - [x] `execute` [execute.c](skeleton2021/execute.c)
    - [x] `parser` [parser.c](skeleton2021/parser.h)
    - [ ] `cash` [mybash.c](skeleton2021/mybash.c)
    - [x] [command.c](skeleton2021/command.c)
- [x] Modificar el parser.c