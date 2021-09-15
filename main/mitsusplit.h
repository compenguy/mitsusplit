#ifndef __mitsusplit_h__
#define __mitsusplit_h__

// Synchronous (blocking) function for provisioning wifi, then connecting to
// wifi and returning
//
// Aborts on error
void ensure_provisioned();

// Synchronous (blocking) function for starting mqtt client, connecting to
// broker and returning (with subsequent asynchronous mqtt message handling)
void start_mqtt();

#endif // __mitsusplit_h__
