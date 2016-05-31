#pragma prototyped

/*
 * codex aes crypt stub
 */

#include <codex.h>

static int
aes_open(Codex_t* p, char* const args[], Codexnum_t flags)
{
	return -1;
}

static int
aes_close(Codex_t* p)
{
	return -1;
}

static int
aes_init(Codex_t* p)
{
	return -1;
}

static int
aes_done(Codex_t* p)
{
	return -1;
}

static ssize_t
aes_read(Sfio_t* sp, void* buf, size_t n, Sfdisc_t* disc)
{
	return -1;
}

static ssize_t
aes_write(Sfio_t* sp, const void* buf, size_t n, Sfdisc_t* disc)
{
	return -1;
}

Codexmeth_t	codex_aes =
{
	"aes",
	"AES Stub",
	"[+(author)?Lefteris Koutsofios]",
	CODEX_DECODE|CODEX_ENCODE|CODEX_CRYPT,
	0,
	0,
	aes_open,
	aes_close,
	aes_init,
	aes_done,
	aes_read,
	aes_write,
	aes_done,
	0,
	0,
	0,
	0,
	CODEXNEXT(aes)
};

CODEXLIB(aes)
