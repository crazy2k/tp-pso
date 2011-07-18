.. default-role:: math

El sistema por dentro
=====================

El sistema operativo de este proyecto consta de un *kernel* monolítico muy
sencillo. Algunas de las características del kernel, al momento de escribir
este documento, son:

* **carga de programas** desde ejecutables con un formato propio (PSO)
* **protección de memoria** utilizando paginación y espacios de memoria
  diferentes para las tareas
* manejador de interrupciones que permite el **registro de
  handlers en tiempo de ejecución**
* **scheduling de procesos utilizando round-robin** con *quantum* fijo
* **task-switching por software**
* atención de **llamadas al sistema**
* **controladores** de consolas, puerto serie y disco rígido ATA
* **sistema de archivos virtual** (primitivo) y soporte para lectura de
  ext2

En este documento se describen estas y otras características del sistema.

La estructura de directorios
----------------------------

Si bien la parte más importante del proyecto está escrita en C, en realidad,
este también se compone de archivos con código *assembly* de x86, *scripts* de
utilidad y de configuración de herramientas, archivos binarios y
documentación. Todo estos archivos están organizados en una estructura de
directorios que facilita su búsqueda.

Esta es una síntesis de la estructura de directorios del proyecto:

.. graphviz::

    digraph direcciones {
        "raíz" [shape=box]
        "src/" [shape=box]
        "doc/" [shape=box]
        "bin/" [shape=box]
        "boot/" [shape=box]
        "include/" [shape=box]
        "kernel/" [shape=box]
        "tasks/" [shape=box]
        "raíz" -> "src/"
        "raíz" -> "doc/"
        "src/" -> "bin/"
        "src/" -> "boot/"
        "src/" -> "include/"
        "src/" -> "kernel/"
        "src/" -> "tasks/"
    }

================ ==========================================================
Directorio       Contenido
================ ==========================================================
**raíz           representa la base de la estructura de directorios. Además
                 de contener al resto de los directorios, posee archivos de
                 alcance global en el proyecto.
**src/**         posee todos los archivos y directorios referidos al
                 proyecto menos la documentación
**src/bin/**     es el directorio en el que se hallará la imagen del kernel
                 y los archivos correspondientes al disco floppy y disco
                 rígido una vez se realice la compilación del código.
                 Además, posee el código binario correspondiente a los BIOS
                 de bochs.
**src/boot/**    alberga el código fuente del bootloader.
**src/include/** posee los encabezados del kernel.
**src/kernel/**  contiene el código fuente del kernel y además alberga los
                 archivos objeto resultantes de su compilación.
**src/tasks/**   contiene el código fuente y objeto de las tareas de
                 usuario y también archivos relacionados con el formato de
                 ejecutable propio.
**doc/**         contiene los fuentes de la documentación y algunas
                 utilidades para construirla.
================ ==========================================================


Manejo de procesos
------------------

La ejecución y el manejo de tareas (o procesos [1]_) es una parte
importante de cualquier sistema operativo. Este sistema no es la excepción.

El manejo de las tareas se encuentra repartido en los módulos ``sched``
(``src/kernel/sched.c``) y ``loader`` (``src/kernel/loader.c``). El primero se
ocupa estrictamente del algoritmo de *scheduling*, mientras que el segundo se
encarga de la creación efectiva de los procesos, de los cambios de contexto,
entre otras cosas.

.. [1] Usamos los términos "tarea" y "proceso" de manera
       intercambiable.

El descriptor de proceso
~~~~~~~~~~~~~~~~~~~~~~~~

Cada tarea es representada en el *scheduler* (``sched``) por una
estructura ``sched_task`` cuya definición puede hallarse en
``kernel/sched.c``::

    struct sched_task {
        bool blocked;
        uint32_t quantum;
        uint32_t rem_quantum;

        sched_task *next;
        sched_task *prev;
    };


Esta estructura contiene todo lo que el scheduler precisa saber sobre la
tarea. Entre estas cosas, se encuentra el quantum asociado a la tarea, el
quantum que le queda por consumir y un *flag* que indica si la tarea se
encuentra bloqueada.

Por otro lado, el módulo ``loader`` define una estructura ``pcb`` para cada
tarea. La definición de esta estructura se encuentra en ``kernel/loader.c``::

    struct pcb {
        // Direccion virtual y fisica del directorio de paginas en cualquier
        // espacio de direcciones
        void *pd;

        // Datos sobre el stack de kernel de la tarea
        void *kernel_stack;
        void *kernel_stack_limit;
        void *kernel_stack_pointer;

        // File descriptors
        chardev *fds[MAX_FD];
        uint32_t last_fd;

        pcb *next;
        pcb *prev;
    };

Esta estructura contiene datos sobre el *stack* de kernel de la tarea (su
dirección virtual, el valor del puntero del stack), la dirección del
directorio de páginas de la tarea y el arreglo de archivos abiertos por
la tarea.

El *scheduler*
~~~~~~~~~~~~~~

El algoritmo de *scheduling* utilizado es extremadamente sencillo:
*round-robin* con quantums fijos. A cada tarea se le asigna,
inicialmente, un número fijo de unidades de tiempo para su ejecución.
Cada unidad de tiempo equivale a una interrupción del *timer*.

Las tareas pueden estar bloqueadas (``blocked = TRUE``) o disponibles
para ser ejecutadas (``blocked = FALSE``). Cuando están disponibles,
pueden encontrarse en ejecución o a la espera de su turno. Como el
kernel sólo maneja un único procesador, en todo momento hay a lo sumo
una única tarea en ejecución.

La administración se realiza mediante una lista doblemente enlazada
circular de procesos. La cabeza de esta lista es siempre el proceso
actualmente en ejecución. Cuando se acaba el quantum de una tarea, la
cabeza pasa a ser la siguiente tarea en la lista que se encuentre
en condiciones de ser ejecutada. Cuando una tarea finaliza su ejecución
(por ejemplo, al invocar ella misma a la llamada al sistema ``exit()``)
esta es quitada de la lista y los recursos que utilizaba son liberados.

El scheduler exporta una función para cada tipo de evento:

* ``sched_load()`` para la carga de la tarea,
* ``sched_block()`` y ``sched_unblock()`` para los eventos de bloqueo y
  desbloqueo de la tarea,
* ``sched_tick()`` para la ocurrencia de un *tick* del timer,
* ``sched_exit()`` para la terminación de una tarea.

Creación de tareas
~~~~~~~~~~~~~~~~~~

Una de las primeras tareas que ingresa al sistema es la tarea "idle".
Esta tarea se ejecuta en el anillo de kernel y su sola función es
quedarse a la espera de una interrupción sin consumir recursos del
procesador. El código de esta tarea se encuentre en el archivo
``kernel/loader_helpers.asm``, bajo la etiqueta ``idle_main``.

El ``loader`` (cuyo código se encuentra en ``kernel/loader.c``) se
ocupa de iniciar la creación de las tareas. Para que el kernel comience
la carga de una tarea, se utiliza la función ``loader_load()``. Esta
función se encarga de inicializar un ``pcb`` para la tarea, creándole
su directorio de páginas inicial y reservando memoria para su stack de
modo kernel. Además, prepara en dicho stack un estado inicial para la
tarea y escribe en él la dirección de la función ``initialize_task``
que será el primer código que ejecutará la tarea. Como paso final, se
realiza la llamada a ``sched_load()`` para avisar al scheduler de la
llegada de la tarea.

Hasta este punto, se reserva espacio para el descriptor del proceso en
los módulos ``sched`` y ``loader``, para el stack de kernel de la
tarea, para su directorio de páginas, pero la reserva y mapeo del stack
de usuario y del código y los datos de la tarea en su espacio de
direcciones virtual, utilizando la información en el ejecutable
correspondiente, se realiza recién cuando a esta le toca ejecutarse por
primera vez. La función ``initialize_task`` es justamente la encargada
de realizar esto. El código de dicha función se encuentra en
``kernel/loader_helpers.asm``.

Cambios de contexto
~~~~~~~~~~~~~~~~~~~

El kernel realiza los cambios de contexto de las tareas por *software*.
Como consecuencia, hay una única TSS que se utiliza al mínimo: Sólo
se utilizan el campo correspondiente al selector de segmento del
stack en modo kernel (``SS0``) y el correspondiente al *stack pointer*
en modo kernel (``ESP0``). Estos campos de la TSS son utilizados por el
*hardware* para cargar los registros ``SS`` y ``ESP0`` respectivamente
al ocurrir un cambio de nivel al nivel 0.

Los contextos de las tareas son resguardados en sus correspondientes
stacks de kernel. Al ocurrir una interrupción mientras se está
ejecutando una tarea, el *handler* de la interrupción toma el control.
Si se precisa un cambio de contexto o si el kernel precisará el estado
actual de la tarea para algo [2]_, el handler almacena el contexto de
la tarea en el stack de modo kernel y luego llama a la rutina de
atención correspondiente (ver la sección `Manejo de interrupciones`_).

Si la interrupción no deriva en un cambio de contexto, al terminar de
manejarla, simplemente se procede de manera inversa, cargando el estado
de la tarea desde el stack de kernel y volviendo a ejecutar en modo
usuario. Sin embargo, si la interrupción sí derivará en un cambio de
contexto, se realiza la llamada a ``loader_switchto`` que procede del
siguiente modo:

* guarda el registro ``EFLAGS`` (*flags* del procesador)
* se carga el espacio de direcciones de la nueva tarea,
* se actualizan los valores de ``SS0`` y ``ESP0`` en la TSS del sistema
* se almacena el *stack pointer* de modo kernel actual en el ``pcb``
  de la tarea que estaba ejecutando y se carga el correspondiente a la
  nueva tarea desde su ``pcb``

La función ``loader_switchto()`` (``kernel/loader.c``) se ocupa de lo
anterior, y para el último punto utiliza la función
``loader_switch_stack_pointers()`` definida en ``kernel/loader_helpers.asm``.
Al retornar de dicha función, se buscará la dirección de retorno en este
"nuevo" stack. Si la tarea ya había estado en ejecución, simplemente irá
retornando hasta llegar a la parte en la que se carga el contexto desde el
stack y se vuelve a ejecutar en modo usuario. No obstante, si la tarea es una
tarea nueva, su stack fue armado cuidadosamente de manera que al retornar de
la función se ejecute el código del label ``initialize_task``. Esta porción de
código es la encargada de reservar memoria y realizar los mapeos que ya se
nombraron antes en `Creación de tareas`_.

.. [2] En el estado actual del código, *siempre* se resguarda el estado de la
       tarea independientemente de si el kernel lo precisará o no.

Manejo de interrupciones
------------------------

La función ``idt_init()`` en ``kernel/idt.c`` se ocupa de
inicializar el módulo de manejo de interrupciones. Para esto, escribe
los descriptores en la IDT para las interrupciones que serán manejadas.

Puede designarse el handler que se desee para cualquier índice en la IDT. Sin
embargo, existe un arreglo de handlers llamado ``idt_stateful_handlers`` que
puede ser aprovechado. Los handlers en este arreglo son generados en
``kernel/isr.asm``. La razón por la que hay un *handler* diferente por cada
interrupción es porque es la única forma de poder establecer qué interrupción
se está atendiendo, ya que, cuando ocurre una interrupción, se ejecuta el
código cuya dirección fue registrada en la IDT, pero el hardware no almacena
información que permita identificar de qué interrupción se trata.

Estas rutinas se encargan de guardar el contexto de la tarea en
ejecución en el stack de modo kernel y luego llaman a una función
común, llamada ``idt_handle()``, pasándole a esta el índice en la IDT
de la interrupción ocurrida, un código de error si existiera y el
contexto guardado. De allí en más, ``idt_handle()`` puede ocuparse de delegar
el manejo de la interrupción en rutinas de servicio, escritas en C.

Una vez que ``idt_init()`` escribió la IDT, da aviso al procesador de que
tiene la IDT lista. Por último, configura los PIC y desenmascara sólo las
interrupciones de *hardware* que le interesarán al kernel.

Atención de llamadas al sistema
-------------------------------

Las llamadas al sistema se realizan a través de la interrupción
``0x30``. Antes de generar la interrupción por software, el proceso
debe escribir el número correspondiente a la llamada al sistema que
desea ejecutar en el registro ``EAX``. Los parámetros de la llamada al
sistema deben pasarse usando los registros ``EBX``, ``ECX`` y ``EDX``.
Al ocurrir la interrupción, la rutina de servicio de la interrupción
``0x30`` llama a la función correspondiente a la llamada al sistema
invocada con los parámetros pasados.

Las llamadas al sistema implementadas hasta ahora son:

======= =============== ===============================================
Número  Nombre          Función
======= =============== ===============================================
1       ``exit()``      finaliza el proceso en ejecución y libera todos
                        los recursos utilizados por este
2       ``getpid()``    devuelve el identificador de proceso de la
                        tarea
3       ``palloc()``    reserva una página de memoria para la tarea
4       ``read()``      lee de un archivo abierto
5       ``write()``     escribe en un archivo abierto
6       ``seek()``      traslada el puntero del archivo a una posición
                        dada
7       ``close()``     cierra el archivo y libera los recursos
                        que ya no se precisen
8       ``open()``      abre un archivo para lectura y/o escritura
9       ``con_ctl()``   envía comandos de control a una consola
10      ``run()``       pone un programa en ejecución
======= =============== ===============================================

La memoria
----------

Gestión de la memoria
~~~~~~~~~~~~~~~~~~~~~

El módulo encargado de la gestión de la memoria es el módulo ``mm``
(``kernel/mm.c``). Su función de inicialización, ``mm_init()``, se encarga de

1. armar la lista de páginas libres para el kernel y para usuario,
2. inicializar un directorio de páginas para el kernel y 
3. activar la paginación de memoria.

Para el paso 1, recorre la memoria verificando qué páginas de memoria son
válidas y arma dos listas de páginas libres: las del kernel, que están por
debajo de los 4MB de memoria física, y las de usuario.

Cada página de memoria física disponible está representada por una estructura
``page_t``.  Dicha estructura se encuentra declarada en ``kernel/mm.c`` del
siguiente modo::

    struct page_t {
        int count;

        page_t *next;
        page_t *prev;
    };

Los punteros ``next`` y ``prev`` son utilizados para administrar las
listas de páginas físicas libres y``count`` indica el número de
referencias de la página.

En el módulo ``mm`` se encuentran todas las funciones que se ocupan
de gestionar las páginas físicas libres y de mapearlas a los espacios
de direcciones virtuales.

Direccionamiento
~~~~~~~~~~~~~~~~

Una de las primeras cosas que se realizan en ``kernel/kinit.asm``
(el código al que salta el bootloader) es configurar una GDT definitiva
para el sistema. La misma está compuesta por descriptores para:

1. Código de nivel 0
2. Datos de nivel 0
3. Código de nivel 3
4. Datos de nivel 3
5. TSS del sistema

Tanto los segmentos de código como de datos ocupan todo el espacio
direccionable. El principal mecanismo de protección de memoria
utilizado en el kernel es la paginación.

Cada tarea cuenta con un espacio de direcciones virtual propio, pero todas
ellas tienen al código y los datos del kernel mapeados en las direcciones
bajas (*lower half*) mientras que el código y los datos de usuario se
encuentran en direcciones más altas.

*Devices*
---------

El *device* es una abstracción que permite manipular dispositivos de
*hardware* y estructuras lógicas con facilidad y mediante una interfaz
común.

Cada tipo de device está representado por una estructura específica.
Sin embargo, todas estas estructuras "derivan de" una estructura en
particular. En este contexto, una estructura deriva de otra si todos
los campos de esta última están incluidos al principio (y en el mismo
orden) en la primera. La estructura más general de device, definida en
``include/device.h``, es::

    struct device {
        uint32_t clase;
        uint32_t refcount;
        dev_flush_t *flush;
    };

La ``clase`` define de qué tipo de device se trata y, consecuentemente,
cuál es la estructura asociada. ``refcount`` indica el número de
referencias, es decir, la cantidad de usuarios del device. Por último,
``flush`` es una función que se llama cuando el device dejará de ser
utilizado y se encarga de liberar los recursos utilizados por el mismo.

Las principales estructuras que derivan de ``device`` son ``chardev`` y
``blockdev``. La primera estructura representa un device que puede
accederse comúnmente como una tira de bytes. La segunda representa uno
que puede acceders2e de a porciones definidas (bloques)::

    struct chardev {
        uint32_t clase;
        uint32_t refcount;
        chardev_flush_t *flush;
        chardev_read_t *read;
        chardev_write_t *write;
        chardev_seek_t *seek;
    };

    struct blockdev {
        uint32_t clase;
        uint32_t refcount;
        blockdev_flush_t *flush;
        blockdev_read_t *read;
        blockdev_write_t *write;
        uint32_t size;
    };

Ambas respetan los campos de la estructura ``device`` por "derivar" de
ella. La diferencia entre las funciones de lectura y escritura de los
``chardev`` y los ``blockdev`` es, básicamente, que para los primeros
se asume que existe una posición actual, fijada con ``seek``, (o que no
es relevante especificar una posición) a partir de la cual leer,
mientras que para los segundos la posición debe especificarse y
normalmente indica un número de bloque.

Las consolas
~~~~~~~~~~~~

El módulo ``con`` implementa el anillo de consolas. Las consolas son
esencialmente *buffers* de memoria destinados a ser representados en
la pantalla como texto. En todo momento hay una consola que se
encuentra "en foco", es decir, visible en la pantalla.

Cada consola está representada por una estructura ``con_chardev``,
definida en ``include/con.h``::

    struct con_chardev {
        uint32_t clase;
        uint32_t refcount;
        chardev_flush_t* flush;
        chardev_read_t* read;
        chardev_write_t* write;
        chardev_seek_t* seek;

        void *screen_buf;
        uint32_t screen_buf_offset;

        circular_buf_t kb_buf;
        int waiting_process;

        uint8_t current_attr;

        con_chardev *next;
        con_chardev *prev;
    }

Esta estructura alberga la dirección virtual del buffer de la consola y
la posición actual en este buffer en ``screen_buf`` y
``screen_buf_offset`` respectivamente. Al realizarse una escritura
sobre el device, llamando a su funcion ``write``, se almacenan los
datos en el buffer y se avanza el puntero en ``screen_buf_offset`` de
manera acorde. Cuando se llega al final del buffer, se produce un
desplazamiento de los datos que consiste en mover todo el contenido del
buffer 80 caracteres (una línea) hacia atrás y limpiar los últimos 80
caracteres del buffer.

Si se llama a la función ``read`` de la consola, la tarea en ejecución
queda a la espera de que se oprima una tecla en el teclado. Cuando esto
ocurre, la rutina de atención de la interrupción del teclado avisa al
módulo ``kb`` del evento, el cual se encarga de enviarle al módulo
``con`` el caracter recibido. La función ``con_put_to_kb_buf``
finalmente se encarga de almacenar el caracter en el buffer ``kb_buf``
de la consola en foco y de despertar a la tarea en espera. Al
despertarse la tarea, la función ``read`` ya puede leer del buffer de
teclado los caracteres recibidos y guardarlos donde se le indicó (por
ejemplo, un buffer de usuario).

El puerto serie
~~~~~~~~~~~~~~~

El kernel incluye un controlador para UART (*Universal Asynchronous
Receiver/Transmitter*), que permite leer de y escribir al conocido
"puerto serie". Para representar al dispositivo se utiliza la estructura
``serial_chardev``, definida en ``include/serial.h``.

El funcionamiento para el caso de la lectura es similar al caso de la
consola. La tarea queda bloqueada a la espera de la llegada de un byte.
Cuando este llega, se almacena en el buffer del dispositivo y se
despierta a la tarea para que esta pueda tomar el dato. Para el caso de
la escritura, simplemente se envían al dispositivo uno a uno los bytes
a transmitir, esperando entre cada envío que el dispositivo se
encuentre listo.

Disco rígido ATA IDE
~~~~~~~~~~~~~~~~~~~~

El módulo ``hdd`` se encarga del manejo de los discos ATA IDE. El modo
de acceso a los discos es el de entrada/salida programada. De momento
sólo se ha implementado la funcionalidad de lectura de disco, sin tener
en cuenta los permisos de usuario.

La estructura ``hdd_blockdev`` representa al disco rígido. Respetando
los prototipos de las funciones de los ``blockdev``, la función de
lectura recibe, además de un buffer con su respectivo tamaño, la
posición del bloque a leer, expresada como una dirección LBA (Logical
Block Addressing) de 28 bits. La operatoria es similar a los demás
dispositivos: Se envía el pedido de sectores al disco y se bloquea la
tarea a la espera de recibir los datos en un buffer. El buffer es
llenado (y la tarea desbloqueada) cuando la rutina de atención de la
interrupción del disco se encarga de obtener los datos.

El sistema de archivos virtual
------------------------------

El nombre "sistema de archivos virtual" quizás le queda algo grande a
la rústica implementación que llevamos a cabo, en el módulo ``fs`` para
tener un manejo mínimo de los archivos en el *filesystem* físico y de
los dispositivos de hardware.

La llamada al sistema ``open()`` permite la apertura de un archivo para
su posterior lectura o escritura. La implementación de esta llamada es
la siguiente::

    int sys_open(char *path, uint32_t mode) {
        chardev *cdev;
        if (!(cdev = fs_open(path, mode)))
            return -ENOFILE;
        else {
            cdev->refcount++;
            return loader_add_file(cdev);
        }
    }

Simplemente, la llamada recibe una ruta y un modo de apertura. Ambos
datos se pasan a la función ``fs_open()``, que se encarga de encontrar
un ``chardev`` correspondiente a esa ruta, si lo hay, o devolver
``NULL``. Una vez que se obtuvo el ``chardev`` correspondiente a la
ruta, la función ``loader_add_file()`` agrega el ``chardev`` a la lista
de dispositivos abiertos por la tarea y devuelve un nuevo *file
descriptor*.

Para encontrar el ``chardev``, ``fs_open()`` simplemente respeta
algunas reglas:

* ``/serial0`` y ``/serial1`` representan los ``chardev``
  correspondientes a los "puertos serie";
* ``/console`` representa una nueva consola, y ``/console<i>`` siendo
  ``<i>`` un entero no negativo representa la consola número ``<i>``;
* por último, ``/disk/<ruta>`` representa el archivo en la ruta
  ``<ruta>`` en el filesystem existente en el disco rígido.

Sistema de archivos ext2
~~~~~~~~~~~~~~~~~~~~~~~~

El único sistema de archivos implementado para el disco rígido es ext2,
en su primera versión. Se lo implementó de manera rudimentaria, con
soporte para lectura y sin tener en cuenta permisos.

La implementación actual lee el archivo entero y lo almacena en un
buffer al momento de la apertura. Está en construcción una
implementación que permite la lectura y almacenamiento de los datos del
archivo de a bloques y bajo demanda.
