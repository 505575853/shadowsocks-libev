/* select - Module containing unix select(2) call.
   Under Unix, the file descriptors are small integers.
   Under Win32, select only exists for sockets, and sockets may
   have any value except INVALID_SOCKET.
   Under BeOS, we suffer the same dichotomy as Win32; sockets can be anything
   >= 0.
*/

#include <Python.h>

/* Windows #defines FD_SETSIZE to 64 if FD_SETSIZE isn't already defined.
   64 is too small (too many people have bumped into that limit).
   Here we boost it.
   Users who want even more than the boosted limit should #define
   FD_SETSIZE higher before this; e.g., via compiler /D switch.
*/
#if defined(_WIN32) && !defined(FD_SETSIZE)
#define FD_SETSIZE 512
#endif

#ifdef _WIN32
#  include <winsock2.h>
#endif

static PyObject *SelectError;

/* **************************************************************************
 *                      epoll interface for Linux 2.6
 *
 * Written by Christian Heimes
 * Inspired by Twisted's _epoll.pyx and select.poll()
 */

#include "wepoll.h"

typedef struct {
    PyObject_HEAD
    SOCKET epfd;                        /* epoll control file descriptor */
} pyEpoll_Object;

static PyTypeObject pyEpoll_Type;
#define pyepoll_CHECK(op) (PyObject_TypeCheck((op), &pyEpoll_Type))

static PyObject *
pyepoll_err_closed(void)
{
    PyErr_SetString(PyExc_ValueError, "I/O operation on closed epoll fd");
    return NULL;
}

static int
pyepoll_internal_close(pyEpoll_Object *self)
{
    int save_errno = 0;
    if (self->epfd >= 0) {
        int epfd = self->epfd;
        self->epfd = -1;
        Py_BEGIN_ALLOW_THREADS
        if (epoll_close(epfd) < 0)
            save_errno = errno;
        Py_END_ALLOW_THREADS
    }
    return save_errno;
}

static PyObject *
newPyEpoll_Object(PyTypeObject *type, int sizehint, SOCKET fd)
{
    pyEpoll_Object *self;

    if (sizehint == -1) {
        sizehint = FD_SETSIZE-1;
    }
    else if (sizehint < 1) {
        PyErr_Format(PyExc_ValueError,
                     "sizehint must be greater zero, got %d",
                     sizehint);
        return NULL;
    }

    assert(type != NULL && type->tp_alloc != NULL);
    self = (pyEpoll_Object *) type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    if (fd == -1) {
        Py_BEGIN_ALLOW_THREADS
        self->epfd = epoll_create(sizehint);
        Py_END_ALLOW_THREADS
    }
    else {
        self->epfd = fd;
    }
    if (self->epfd < 0) {
        Py_DECREF(self);
        PyErr_SetFromErrno(PyExc_IOError);
        return NULL;
    }
    return (PyObject *)self;
}


static PyObject *
pyepoll_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int sizehint = -1;
    static char *kwlist[] = {"sizehint", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|i:epoll", kwlist,
                                     &sizehint))
        return NULL;

    return newPyEpoll_Object(type, sizehint, -1);
}


static void
pyepoll_dealloc(pyEpoll_Object *self)
{
    (void)pyepoll_internal_close(self);
    Py_TYPE(self)->tp_free(self);
}

static PyObject*
pyepoll_close(pyEpoll_Object *self)
{
    errno = pyepoll_internal_close(self);
    if (errno < 0) {
        PyErr_SetFromErrno(PyExc_IOError);
        return NULL;
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(pyepoll_close_doc,
"close() -> None\n\
\n\
Close the epoll control file descriptor. Further operations on the epoll\n\
object will raise an exception.");

static PyObject*
pyepoll_get_closed(pyEpoll_Object *self)
{
    if (self->epfd < 0)
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

static PyObject*
pyepoll_fileno(pyEpoll_Object *self)
{
    if (self->epfd < 0)
        return pyepoll_err_closed();
    return PyInt_FromLong(self->epfd);
}

PyDoc_STRVAR(pyepoll_fileno_doc,
"fileno() -> int\n\
\n\
Return the epoll control file descriptor.");

static PyObject*
pyepoll_fromfd(PyObject *cls, PyObject *args)
{
    SOCKET fd;

    if (!PyArg_ParseTuple(args, "i:fromfd", &fd))
        return NULL;

    return newPyEpoll_Object((PyTypeObject*)cls, -1, fd);
}

PyDoc_STRVAR(pyepoll_fromfd_doc,
"fromfd(fd) -> epoll\n\
\n\
Create an epoll object from a given control fd.");

static PyObject *
pyepoll_internal_ctl(int epfd, int op, PyObject *pfd, unsigned int events)
{
    struct epoll_event ev;
    int result;
    int fd;

    if (epfd < 0)
        return pyepoll_err_closed();

    fd = PyObject_AsFileDescriptor(pfd);
    if (fd == -1) {
        return NULL;
    }

    switch(op) {
        case EPOLL_CTL_ADD:
        case EPOLL_CTL_MOD:
        ev.events = events;
        ev.data.fd = fd;
        Py_BEGIN_ALLOW_THREADS
        result = epoll_ctl(epfd, op, fd, &ev);
        Py_END_ALLOW_THREADS
        break;
        case EPOLL_CTL_DEL:
        /* In kernel versions before 2.6.9, the EPOLL_CTL_DEL
         * operation required a non-NULL pointer in event, even
         * though this argument is ignored. */
        Py_BEGIN_ALLOW_THREADS
        result = epoll_ctl(epfd, op, fd, &ev);
        if (errno == EBADF) {
            /* fd already closed */
            result = 0;
            errno = 0;
        }
        Py_END_ALLOW_THREADS
        break;
        default:
        result = -1;
        errno = EINVAL;
    }

    if (result < 0) {
        PyErr_SetFromErrno(PyExc_IOError);
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
pyepoll_register(pyEpoll_Object *self, PyObject *args, PyObject *kwds)
{
    PyObject *pfd;
    unsigned int events = EPOLLIN | EPOLLOUT | EPOLLPRI;
    static char *kwlist[] = {"fd", "eventmask", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|I:register", kwlist,
                                     &pfd, &events)) {
        return NULL;
    }

    return pyepoll_internal_ctl(self->epfd, EPOLL_CTL_ADD, pfd, events);
}

PyDoc_STRVAR(pyepoll_register_doc,
"register(fd[, eventmask]) -> None\n\
\n\
Registers a new fd or raises an IOError if the fd is already registered.\n\
fd is the target file descriptor of the operation.\n\
events is a bit set composed of the various EPOLL constants; the default\n\
is EPOLL_IN | EPOLL_OUT | EPOLL_PRI.\n\
\n\
The epoll interface supports all file descriptors that support poll.");

static PyObject *
pyepoll_modify(pyEpoll_Object *self, PyObject *args, PyObject *kwds)
{
    PyObject *pfd;
    unsigned int events;
    static char *kwlist[] = {"fd", "eventmask", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "OI:modify", kwlist,
                                     &pfd, &events)) {
        return NULL;
    }

    return pyepoll_internal_ctl(self->epfd, EPOLL_CTL_MOD, pfd, events);
}

PyDoc_STRVAR(pyepoll_modify_doc,
"modify(fd, eventmask) -> None\n\
\n\
fd is the target file descriptor of the operation\n\
events is a bit set composed of the various EPOLL constants");

static PyObject *
pyepoll_unregister(pyEpoll_Object *self, PyObject *args, PyObject *kwds)
{
    PyObject *pfd;
    static char *kwlist[] = {"fd", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O:unregister", kwlist,
                                     &pfd)) {
        return NULL;
    }

    return pyepoll_internal_ctl(self->epfd, EPOLL_CTL_DEL, pfd, 0);
}

PyDoc_STRVAR(pyepoll_unregister_doc,
"unregister(fd) -> None\n\
\n\
fd is the target file descriptor of the operation.");

static PyObject *
pyepoll_poll(pyEpoll_Object *self, PyObject *args, PyObject *kwds)
{
    double dtimeout = -1.;
    int timeout;
    int maxevents = -1;
    int nfds, i;
    PyObject *elist = NULL, *etuple = NULL;
    struct epoll_event *evs = NULL;
    static char *kwlist[] = {"timeout", "maxevents", NULL};

    if (self->epfd < 0)
        return pyepoll_err_closed();

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|di:poll", kwlist,
                                     &dtimeout, &maxevents)) {
        return NULL;
    }

    if (dtimeout < 0) {
        timeout = -1;
    }
    else if (dtimeout * 1000.0 > INT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "timeout is too large");
        return NULL;
    }
    else {
        timeout = (int)(dtimeout * 1000.0);
    }

    if (maxevents == -1) {
        maxevents = FD_SETSIZE-1;
    }
    else if (maxevents < 1) {
        PyErr_Format(PyExc_ValueError,
                     "maxevents must be greater than 0, got %d",
                     maxevents);
        return NULL;
    }

    evs = PyMem_New(struct epoll_event, maxevents);
    if (evs == NULL) {
        Py_DECREF(self);
        PyErr_NoMemory();
        return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
    nfds = epoll_wait(self->epfd, evs, maxevents, timeout);
    Py_END_ALLOW_THREADS
    if (nfds < 0) {
        PyErr_SetFromErrno(PyExc_IOError);
        goto error;
    }

    elist = PyList_New(nfds);
    if (elist == NULL) {
        goto error;
    }

    for (i = 0; i < nfds; i++) {
        etuple = Py_BuildValue("iI", evs[i].data.fd, evs[i].events);
        if (etuple == NULL) {
            Py_CLEAR(elist);
            goto error;
        }
        PyList_SET_ITEM(elist, i, etuple);
    }

    error:
    PyMem_Free(evs);
    return elist;
}

PyDoc_STRVAR(pyepoll_poll_doc,
"poll([timeout=-1[, maxevents=-1]]) -> [(fd, events), (...)]\n\
\n\
Wait for events on the epoll file descriptor for a maximum time of timeout\n\
in seconds (as float). -1 makes poll wait indefinitely.\n\
Up to maxevents are returned to the caller.");

static PyMethodDef pyepoll_methods[] = {
    {"fromfd",          (PyCFunction)pyepoll_fromfd,
     METH_VARARGS | METH_CLASS, pyepoll_fromfd_doc},
    {"close",           (PyCFunction)pyepoll_close,     METH_NOARGS,
     pyepoll_close_doc},
    {"fileno",          (PyCFunction)pyepoll_fileno,    METH_NOARGS,
     pyepoll_fileno_doc},
    {"modify",          (PyCFunction)pyepoll_modify,
     METH_VARARGS | METH_KEYWORDS,      pyepoll_modify_doc},
    {"register",        (PyCFunction)pyepoll_register,
     METH_VARARGS | METH_KEYWORDS,      pyepoll_register_doc},
    {"unregister",      (PyCFunction)pyepoll_unregister,
     METH_VARARGS | METH_KEYWORDS,      pyepoll_unregister_doc},
    {"poll",            (PyCFunction)pyepoll_poll,
     METH_VARARGS | METH_KEYWORDS,      pyepoll_poll_doc},
    {NULL,      NULL},
};

static PyGetSetDef pyepoll_getsetlist[] = {
    {"closed", (getter)pyepoll_get_closed, NULL,
     "True if the epoll handler is closed"},
    {0},
};

PyDoc_STRVAR(pyepoll_doc,
"winselect.wepoll([sizehint=-1])\n\
\n\
Returns an epolling object\n\
\n\
sizehint must be a positive integer or -1 for the default size. The\n\
sizehint is used to optimize internal data structures. It doesn't limit\n\
the maximum number of monitored events.");

static PyTypeObject pyEpoll_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "winselect.wepoll",                                     /* tp_name */
    sizeof(pyEpoll_Object),                             /* tp_basicsize */
    0,                                                  /* tp_itemsize */
    (destructor)pyepoll_dealloc,                        /* tp_dealloc */
    0,                                                  /* tp_print */
    0,                                                  /* tp_getattr */
    0,                                                  /* tp_setattr */
    0,                                                  /* tp_compare */
    0,                                                  /* tp_repr */
    0,                                                  /* tp_as_number */
    0,                                                  /* tp_as_sequence */
    0,                                                  /* tp_as_mapping */
    0,                                                  /* tp_hash */
    0,                                                  /* tp_call */
    0,                                                  /* tp_str */
    PyObject_GenericGetAttr,                            /* tp_getattro */
    0,                                                  /* tp_setattro */
    0,                                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                                 /* tp_flags */
    pyepoll_doc,                                        /* tp_doc */
    0,                                                  /* tp_traverse */
    0,                                                  /* tp_clear */
    0,                                                  /* tp_richcompare */
    0,                                                  /* tp_weaklistoffset */
    0,                                                  /* tp_iter */
    0,                                                  /* tp_iternext */
    pyepoll_methods,                                    /* tp_methods */
    0,                                                  /* tp_members */
    pyepoll_getsetlist,                                 /* tp_getset */
    0,                                                  /* tp_base */
    0,                                                  /* tp_dict */
    0,                                                  /* tp_descr_get */
    0,                                                  /* tp_descr_set */
    0,                                                  /* tp_dictoffset */
    0,                                                  /* tp_init */
    0,                                                  /* tp_alloc */
    pyepoll_new,                                        /* tp_new */
    0,                                                  /* tp_free */
};

static PyMethodDef select_methods[] = {
    {0,         0},     /* sentinel */
};

PyDoc_STRVAR(module_doc,
"This module supports asynchronous I/O on multiple file descriptors.\n\
\n\
*** IMPORTANT NOTICE ***\n\
On Windows and OpenVMS, only sockets are supported; on Unix, all file descriptors.");

PyMODINIT_FUNC
initwinselect(void)
{
    PyObject *m;
    m = Py_InitModule3("winselect", select_methods, module_doc);
    if (m == NULL)
        return;

    SelectError = PyErr_NewException("winselect.error", NULL, NULL);
    Py_INCREF(SelectError);
    PyModule_AddObject(m, "error", SelectError);

#ifdef PIPE_BUF
#ifdef HAVE_BROKEN_PIPE_BUF
#undef PIPE_BUF
#define PIPE_BUF 512
#endif
    PyModule_AddIntConstant(m, "PIPE_BUF", PIPE_BUF);
#endif

    Py_TYPE(&pyEpoll_Type) = &PyType_Type;
    if (PyType_Ready(&pyEpoll_Type) < 0)
        return;

    Py_INCREF(&pyEpoll_Type);
    PyModule_AddObject(m, "wepoll", (PyObject *) &pyEpoll_Type);

    PyModule_AddIntConstant(m, "EPOLLIN", EPOLLIN);
    PyModule_AddIntConstant(m, "EPOLLOUT", EPOLLOUT);
    PyModule_AddIntConstant(m, "EPOLLPRI", EPOLLPRI);
    PyModule_AddIntConstant(m, "EPOLLERR", EPOLLERR);
    PyModule_AddIntConstant(m, "EPOLLHUP", EPOLLHUP);
#ifdef EPOLLONESHOT
    /* Kernel 2.6.2+ */
    PyModule_AddIntConstant(m, "EPOLLONESHOT", EPOLLONESHOT);
#endif
    /* PyModule_AddIntConstant(m, "EPOLL_RDHUP", EPOLLRDHUP); */
#ifdef EPOLLRDNORM
    PyModule_AddIntConstant(m, "EPOLLRDNORM", EPOLLRDNORM);
#endif
#ifdef EPOLLRDBAND
    PyModule_AddIntConstant(m, "EPOLLRDBAND", EPOLLRDBAND);
#endif
#ifdef EPOLLWRNORM
    PyModule_AddIntConstant(m, "EPOLLWRNORM", EPOLLWRNORM);
#endif
#ifdef EPOLLWRBAND
    PyModule_AddIntConstant(m, "EPOLLWRBAND", EPOLLWRBAND);
#endif
#ifdef EPOLLMSG
    PyModule_AddIntConstant(m, "EPOLLMSG", EPOLLMSG);
#endif
}
