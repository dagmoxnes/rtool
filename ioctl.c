#include <stdint.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <stdlib.h>

#include "rdma_user_ioctl_cmds.h"
#include "ib_user_ioctl_cmds.h"

static int get_object_handles(int fd, int obj_type, int max_count,
			      uint32_t *handles,
			      uint32_t *ret_count)
{
	struct cmd {
		struct ib_uverbs_ioctl_hdr hdr;
		struct ib_uverbs_attr attrs[3];
	};

	struct cmd op = {
		.hdr = {
	       		.length = sizeof(op),
	       		.object_id = UVERBS_OBJECT_DEVICE,
	       		.method_id = UVERBS_METHOD_INFO_HANDLES,
	       		.num_attrs = 3,
	       		.driver_id = RDMA_DRIVER_MLX5,
	       	},
		.attrs[0] = {
	       		.attr_id = UVERBS_ATTR_INFO_HANDLES_LIST,
	       		.flags = UVERBS_ATTR_F_MANDATORY,
	       		.data.data = (uintptr_t)handles,
			.len = max_count * sizeof(*handles),
	       	},
		.attrs[1] = {
	       		.attr_id = UVERBS_ATTR_INFO_OBJECT_ID,
	       		.flags = UVERBS_ATTR_F_MANDATORY,
	       		.data.data = obj_type,
			.len = sizeof(uint64_t),
	       	},
		.attrs[2] = {
	       		.attr_id = UVERBS_ATTR_INFO_TOTAL_HANDLES,
	       		.flags = UVERBS_ATTR_F_MANDATORY,
	       		.data.data = (uintptr_t)ret_count,
			.len = sizeof(uint32_t),
	       	},

	};

	if (ioctl(fd, RDMA_VERBS_IOCTL, &op))
	         return errno;
	return 0;
}

/**
 * rdma_core_get_obj_handles - Get list of MR handles.
 * @fd:		file descriptor of the ucontext shared via fd sharing
 * @max_count:	maximum number of handles that caller likes to
 * 		receive in the response.
 * @handles:	Pointer to handles, where array will of handles will be
 * 		allocated.
 * @ret_count:	Number of handles filled up in the handles array.
 * 		This could be same or less than the value specificed in
 * 		max_count when API returns success.
 * rdma_core_get_obj_handles() allocates and returns pointer to handles array
 * when it returns succussful completion. max_count could be a any large
 * reasonable value. Caller must free memory returned at handles once caller
 * is done accessing handles array.
 */
int rdma_core_get_obj_handles(int fd, int max_count, uint32_t obj_type,
			      uint32_t **handles, uint32_t *ret_count)
{
	uint32_t *handles_array;
	int ret;

	handles_array = calloc(max_count, sizeof(uint32_t));
	if (!handles_array)
		return -ENOMEM;

	ret = get_object_handles(fd, obj_type, max_count,
				 handles_array, ret_count);
	if (ret < 0)
		goto err;

	*handles = handles_array;
	return 0;
err:
	free(handles_array);
	return ret;
}

/**
 * rdma_core_destroy_mr_by_handle - destroy mr by its handle.
 *
 * @fd:		file descriptor of the ucontext shared via fd sharing
 * @handle:	handle returned by rdma_core_get_obj_handles()
 *
 * rdma_core_destroy_mr() destroys a MR by its handle. It returns 0 on success
 * and failure error code otherwise.
 */
int rdma_core_destroy_mr_by_handle(int fd, uint32_t handle)
{
	struct cmd {
		struct ib_uverbs_ioctl_hdr hdr;
		struct ib_uverbs_attr attrs[1];
	};

	struct cmd op = {
		.hdr = {
	       		.length = sizeof(op),
	       		.object_id = UVERBS_OBJECT_MR,
	       		.method_id = UVERBS_METHOD_MR_DESTROY,
	       		.num_attrs = 1,
	       		.driver_id = RDMA_DRIVER_MLX5,
	       	},
	         .attrs[0] = {
	       		.attr_id = UVERBS_ATTR_DESTROY_MR_HANDLE,
	       		.flags = UVERBS_ATTR_F_MANDATORY,
	       		.data.data = handle,
	       	},
	};

	if (ioctl(fd, RDMA_VERBS_IOCTL, &op))
	         return errno;
	return 0;
}

