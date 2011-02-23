#ifndef __B64_H__
#define __B64_h__

#ifdef __cplusplus
extern "C" {
#endif

int b64_decode( const unsigned char *inbuf, unsigned char *outbuf, int buflen, int max );

#ifdef __cplusplus
}
#endif

#endif /* __B64_H__ */
