#pragma once

#include <stdint.h>
#include <stddef.h>

#include <atomic.h>
#include <spinlock.h>
#include <system.h>

/**
 * Every kernel object begins with an ob_header as its first member
 * Objects live in a namespace tree
 */

struct ob_object;
struct ob_type;
struct ob_namespace;
struct ob_handle_table;
struct process;

#define OB_NAME_MAX     64
#define OB_MAX_TYPES    64

#define OB_KERNEL_HANDLE_BIT (1ULL << 63)

typedef uint32_t ob_access_t;
typedef uint64_t ob_handle_t;

typedef enum ob_type_id
{
    OB_TYPE_UNKNOWN = 0,

    OB_TYPE_DIRECTORY,
    OB_TYPE_SYMLINK,
    OB_TYPE_FILE,
    OB_TYPE_DEVICE,
    OB_TYPE_PROCESS,
    OB_TYPE_THREAD,
    OB_TYPE_MUTEX,
    OB_TYPE_SEMAPHORE,
    OB_TYPE_EVENT,
    OB_TYPE_TIMER,
    OB_TYPE_SECTION,
    OB_TYPE_PORT,
    OB_TYPE_SOCKET,
    OB_TYPE_PIPE,

    OB_TYPE_USER_MIN = 0x100;
} ob_type_id_t;

/* object flags */

#define OB_FLAG_NAMED (1u << 0) // lives in namespace
#define OB_FLAG_PERMANENT (1u << 1)
#define OB_FLAG_KERNEL_ONLY (1u << 2)
#define OB_FLAG_EXCLUSIVE (1u << 3)

#define OB_HANDLE_INHERIT (1u << 0)
#define OB_HANDLE_KERNEL (1u << 1)

typedef struct ob_header
{
    struct ob_type *type;
    struct ob_namespace *ns;
    struct ob_header *parent;

    list_head_t     ns_link;
    list_head_t     type_link;

    uint32_t        refcnt;
    uint32_t        handle_cnt;

    ob_access_t     default_access;
    uint32_t        flags;

    char            name[OB_NAME_MAX];
} ob_header_t;

typedef struct ob_object
{
    ob_header_t hdr;
} ob_object_t;

typedef struct ob_type_ops
{
    void (*destroy)(struct ob_object *obj);

    void (*on_last_handle)(struct ob_object *obj);

    int (*query_info)(struct ob_object *obj,
                      uint32_t info_class,
                      void *buffer
                      size_t *inout_len);

    int (*set_info)(struct ob_object *obj,
                    uint32_t info_class,
                    const void *buffer,
                    size_t length);

    int (*signal)(struct ob_object *obj, uint32_t bits);
} ob_type_ops_t;

typedef struct ob_type
{
    const char *name;
    ob_type_id_t id;

    size_t obj_size;
    ob_access_t valid_access;
    uint32_t flags;

    const ob_type_ops_t *ops;
} ob_type_t;

typedef struct ob_namespace
{
    spinlock_t lock;
    char name[OB_NAME_MAX];

    struct ob_namespace *parent;

    list_head_t children;
    list_head_t child_link;

    list_head_t objects;
} ob_namespace_t;

typedef struct ob_handle_entry
{
    ob_object_t *obj
    ob_access_t access;
    uint32_t flags;
} ob_handle_entry_t;

typedef struct ob_handle_table
{
    spinlock_t lock;
    ob_handle_entry_t *entries;
    size_t capacity;
    size_t used;
    ob_handle_t next_handle;
} ob_handle_table_t;

void ob_init(void);

int ob_register_type(ob_type_t *type);

ob_type_t *ob_type_by_id(ob_type_id_t id);

ob_type_t *ob_type_by_name(const char *name);

int ob_namespace_create(ob_namespace_t *parent,
                        const char *name,
                        ob_namespace_t **out_ns);

int ob_namespace_root(ob_namespace_t **out_ns);

int ob_open_by_path(const char *path,
                    ob_access_t desired_access,
                    ob_object_t **out_obj);

int ob_insert_named(ob_namespace_t *ns, ob_object_t *obj);

int ob_remove_named(ob_object_t *obj);

int ob_create_object(ob_type_t *type,
                     const char *name,
                     ob_namespace_t *ns,
                     uint32_t obj_flags,
                     ob_object_t **out_obj);

void ob_reference_object(ob_object_t *obj);
void ob_dereference_object(ob_object_t *obj);

int ob_handle_table_init(ob_handle_table_t *table);
void ob_handle_table_destroy(ob_handle_table_t *table);

int ob_handle_create(ob_handle_table_t *table,
                     ob_object_t *obj,
                     ob_access_t access,
                     uint32_t flags,
                     ob_handle_t *out_handle);

int ob_handle_close(ob_handle_table_t *table,
                    ob_handle_t handle);

int ob_reference_handle(ob_handle_table_t *table,
                        ob_handle_t handle,
                        ob_access_t desired_access,
                        ob_object_t **out_obj);

int ob_handle_duplicate(ob_handle_table_t *src_table,
                        ob_handle_table_t *dst_table,
                        ob_handle_t src_handle,
                        ob_access_t new_access,
                        uint32_t new_flags,
                        ob_handle_t *out_new_handle);
                    
int ob_query_info(ob_object_t *obj,
                  uint32_t info_class,
                  void *buffer,
                  size_t *inout_len);

int ob_set_info(ob_object_t *ob,
                uint32_t info_class,
                const void *buffer,
                size_t length);

int ob_signal(ob_object_t *obj, uint32_t bits);

#define OB_HEADER(ptr) (&(ptr)->hdr)

#define OB_CONTAINER_OF(type, field, hdrptr) \
    ((type *)((uint8_t *)(hdrptr) - offsetof(type, field)))