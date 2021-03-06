.. default-role:: math

Convenciones para el código de este trabajo
===========================================

Las siguientes son las convenciones que deben seguirse al escribir código para
este trabajo.

.. note::
    Tener en cuenta que el código se encuentra en transición y estas
    convenciones aún no se respetan a rajatabla.

Encabezados y código
--------------------

Cada módulo ``module`` está compuesto por su **código** en el archivo
``src/kernel/module.c`` y su **header o encabezado** correspondiente en
``src/include/module.h``. Adicionalmente, el módulo puede estar compuesto de
un archivo con **código *assembly***, normalmente llamado
``src/kernel/module_helpers.asm``.

Se seguirá la regla de exportar siempre lo mínimo a los demás módulos. Así, en
el encabezado sólo deberán hallarse las declaraciones de funciones, variables
o estructuras que se decidan exportar por ser de utilidad para otros módulos.
Lo mismo aplica para las macros y definiciones de constantes. Las funciones,
variables, estructuras o macros o constantes que sólo interesan al
módulo, deberán hallarse declaradas y/o definidas únicamente en el
código del módulo y, en el caso de código C, deberá usarse la palabra
``static`` para la declaración de variables y funciones no exportadas.

Orden en el código C
--------------------

Para el orden de todo lo que haya en el código C, se seguirá la siguiente
convención:

1. Todas las directivas ``#include``.
2. Todas las directivas ``#define``.
3. Todas las definiciones de variables exportadas.
4. Todas las declaraciones de funciones externas.
5. Todas las declaraciones de variables externas.
6. Todas las declaraciones de funciones locales.
7. Todas las declaraciones de variables locales.
8. Las definiciones de todas las funciones, comenzando con la función de
   inicialización del módulo si la hubiera.
