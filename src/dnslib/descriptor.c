/*
 * File     descriptor.c
 * Date     12.11.2010 12:06
 * Author:  NLabs: NSD team, modifications by Jan Kadlec
 * Project: CuteDNS
 * Description:   
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>

#include "descriptor.h"

enum desclen { DNSLIB_RRTYPE_DESCRIPTORS_LENGTH = 101 };

/* Taken from RFC 1035, section 3.2.4.  */
static lookup_table_type dns_rrclasses[] = {
	{ DNSLIB_CLASS_IN, "IN" },	/* the Internet */
	{ DNSLIB_CLASS_CS, "CS" },	/* the CSNET class (Obsolete) */
	{ DNSLIB_CLASS_CH, "CH" },	/* the CHAOS class */
	{ DNSLIB_CLASS_HS, "HS" },	/* Hesiod */
	{ 0, NULL }
};

static dnslib_rrtype_descriptor_t dnslib_rrtype_descriptors[DNSLIB_RRTYPE_DESCRIPTORS_LENGTH] = {
    /* 0 */
  	{ 0, NULL, 1, { DNSLIB_RDATA_WF_BINARY } },
  	/* 1 */
  	{ DNSLIB_RRTYPE_A, "A", 1, { DNSLIB_RDATA_WF_A } },
  	/* 2 */
  	{ DNSLIB_RRTYPE_NS, "NS", 1, { DNSLIB_RDATA_WF_COMPRESSED_DNAME } },
  	/* 3 */
  	{ DNSLIB_RRTYPE_MD, "MD", 1,
  	  { DNSLIB_RDATA_WF_UNCOMPRESSED_DNAME } },
  	/* 4 */
  	{ DNSLIB_RRTYPE_MF, "MF", 1,
  	  { DNSLIB_RDATA_WF_UNCOMPRESSED_DNAME } },
  	/* 5 */
  	{ DNSLIB_RRTYPE_CNAME, "CNAME", 1,
  	  { DNSLIB_RDATA_WF_COMPRESSED_DNAME } },
  	/* 6 */
  	{ DNSLIB_RRTYPE_SOA, "SOA", 7,
  	  { DNSLIB_RDATA_WF_COMPRESSED_DNAME, DNSLIB_RDATA_WF_COMPRESSED_DNAME, DNSLIB_RDATA_WF_LONG,
  	    DNSLIB_RDATA_WF_LONG, DNSLIB_RDATA_WF_LONG, DNSLIB_RDATA_WF_LONG, DNSLIB_RDATA_WF_LONG } },
  	/* 7 */
  	{ DNSLIB_RRTYPE_MB, "MB", 1,
  	  { DNSLIB_RDATA_WF_COMPRESSED_DNAME } },
  	/* 8 */
  	{ DNSLIB_RRTYPE_MG, "MG", 1,
  	  { DNSLIB_RDATA_WF_COMPRESSED_DNAME } },
  	/* 9 */
  	{ DNSLIB_RRTYPE_MR, "MR", 1,
  	  { DNSLIB_RDATA_WF_COMPRESSED_DNAME } },
    	/* 10 */
  	{ DNSLIB_RRTYPE_NULL, NULL, 1,
  	  { DNSLIB_RDATA_WF_BINARY } },
  	/* 11 */
  	{ DNSLIB_RRTYPE_WKS, "WKS", 2,
  	  { DNSLIB_RDATA_WF_A, DNSLIB_RDATA_WF_BINARY } },
  	/* 12 */
  	{ DNSLIB_RRTYPE_PTR, "PTR", 1,
  	  { DNSLIB_RDATA_WF_COMPRESSED_DNAME } },
  	/* 13 */
  	{ DNSLIB_RRTYPE_HINFO, "HINFO", 2,
  	  { DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT } },
  	/* 14 */
  	{ DNSLIB_RRTYPE_MINFO, "MINFO", 2,
  	  { DNSLIB_RDATA_WF_COMPRESSED_DNAME, DNSLIB_RDATA_WF_COMPRESSED_DNAME } },
  	/* 15 */
  	{ DNSLIB_RRTYPE_MX, "MX", 2,
  	  { DNSLIB_RDATA_WF_SHORT, DNSLIB_RDATA_WF_COMPRESSED_DNAME } },
  	/* 16 */ /* This is obscure, but I guess there's no other way */
  	{ DNSLIB_RRTYPE_TXT, "TXT", MAXRDATALEN,
  	  { DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT,
  	    DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT,
  	    DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT,
  	    DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT,
  	    DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT,
  	    DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT,
  	    DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT,
  	    DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT,
  	    DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT,
  	    DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT,
  	    DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT,
  	    DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT,
  	    DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT,
  	    DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT,
  	    DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT,
  	    DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT } },
  	/* 17 */
  	{ DNSLIB_RRTYPE_RP, "RP", 2,
  	  { DNSLIB_RDATA_WF_COMPRESSED_DNAME, DNSLIB_RDATA_WF_COMPRESSED_DNAME } },
  	/* 18 */
  	{ DNSLIB_RRTYPE_AFSDB, "AFSDB", 2,
  	  { DNSLIB_RDATA_WF_SHORT, DNSLIB_RDATA_WF_COMPRESSED_DNAME } },
  	/* 19 */
  	{ DNSLIB_RRTYPE_X25, "X25", 1,
  	  { DNSLIB_RDATA_WF_TEXT } },
  	/* 20 */
  	{ DNSLIB_RRTYPE_ISDN, "ISDN", 2,
  	  { DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT } },
  	/* 21 */
  	{ DNSLIB_RRTYPE_RT, "RT", 2,
  	  { DNSLIB_RDATA_WF_SHORT, DNSLIB_RDATA_WF_COMPRESSED_DNAME } },
  	/* 22 */
  	{ DNSLIB_RRTYPE_NSAP, "NSAP", 1,
  	  { DNSLIB_RDATA_WF_BINARY } },
  	/* 23 */
  	{ 23, NULL, 1, { DNSLIB_RDATA_WF_BINARY } },
  	/* 24 */
  	{ DNSLIB_RRTYPE_SIG, "SIG", 9,
  	  { DNSLIB_RDATA_WF_SHORT, DNSLIB_RDATA_WF_BYTE, DNSLIB_RDATA_WF_BYTE, DNSLIB_RDATA_WF_LONG,
  	    DNSLIB_RDATA_WF_LONG, DNSLIB_RDATA_WF_LONG, DNSLIB_RDATA_WF_SHORT,
  	    DNSLIB_RDATA_WF_UNCOMPRESSED_DNAME, DNSLIB_RDATA_WF_BINARY } },
  	/* 25 */
  	{ DNSLIB_RRTYPE_KEY, "KEY", 4,
  	  { DNSLIB_RDATA_WF_SHORT, DNSLIB_RDATA_WF_BYTE, DNSLIB_RDATA_WF_BYTE, DNSLIB_RDATA_WF_BINARY } },
  	/* 26 */
  	{ DNSLIB_RRTYPE_PX, "PX", 3,
  	  { DNSLIB_RDATA_WF_SHORT, DNSLIB_RDATA_WF_UNCOMPRESSED_DNAME,
  	    DNSLIB_RDATA_WF_UNCOMPRESSED_DNAME } },
  	/* 27 */
  	{ 27, NULL, 1, { DNSLIB_RDATA_WF_BINARY } },
  	/* 28 */
  	{ DNSLIB_RRTYPE_AAAA, "AAAA", 1,
  	  { DNSLIB_RDATA_WF_AAAA } },
  	/* 29 */
  	{ DNSLIB_RRTYPE_LOC, "LOC", 1,
  	  { DNSLIB_RDATA_WF_BINARY } },
  	/* 30 */
  	{ DNSLIB_RRTYPE_NXT, "NXT", 2,
  	  { DNSLIB_RDATA_WF_UNCOMPRESSED_DNAME, DNSLIB_RDATA_WF_BINARY } },
  	/* 31 */
  	{ 31, NULL, 1, { DNSLIB_RDATA_WF_BINARY } },
  	/* 32 */
  	{ 32, NULL, 1, { DNSLIB_RDATA_WF_BINARY } },
  	/* 33 */
  	{ DNSLIB_RRTYPE_SRV, "SRV", 4,
  	  { DNSLIB_RDATA_WF_SHORT, DNSLIB_RDATA_WF_SHORT, DNSLIB_RDATA_WF_SHORT,
  	    DNSLIB_RDATA_WF_UNCOMPRESSED_DNAME } },
  	/* 34 */
  	{ 34, NULL, 1, { DNSLIB_RDATA_WF_BINARY } },
  	/* 35 */
  	{ DNSLIB_RRTYPE_NAPTR, "NAPTR", 6,
  	  { DNSLIB_RDATA_WF_SHORT, DNSLIB_RDATA_WF_SHORT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT,
  	    DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_UNCOMPRESSED_DNAME } },
  	/* 36 */
  	{ DNSLIB_RRTYPE_KX, "KX", 2,
  	  { DNSLIB_RDATA_WF_SHORT, DNSLIB_RDATA_WF_UNCOMPRESSED_DNAME } },
  	/* 37 */
  	{ DNSLIB_RRTYPE_CERT, "CERT", 4,
  	  { DNSLIB_RDATA_WF_SHORT, DNSLIB_RDATA_WF_SHORT, DNSLIB_RDATA_WF_BYTE, DNSLIB_RDATA_WF_BINARY } },
  	/* 38 */
  	{ DNSLIB_RRTYPE_A6, NULL, 1, { DNSLIB_RDATA_WF_BINARY } },
  	/* 39 */
  	{ DNSLIB_RRTYPE_DNAME, "DNAME", 1,
  	  { DNSLIB_RDATA_WF_UNCOMPRESSED_DNAME } },
  	/* 40 */
  	{ 40, NULL, 1, { DNSLIB_RDATA_WF_BINARY } },
  	/* 41 */
  	{ DNSLIB_RRTYPE_OPT, "OPT", 1,
  	  { DNSLIB_RDATA_WF_BINARY } },
  	/* 42 */
  	{ DNSLIB_RRTYPE_APL, "APL", MAXRDATALEN,
  	  { DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL,
  	    DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL,
  	    DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL,
  	    DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL,
  	    DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL,
  	    DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL,
  	    DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL,
  	    DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL,
  	    DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL,
  	    DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL,
  	    DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL,
  	    DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL,
  	    DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL,
  	    DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL,
  	    DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL,
  	    DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL, DNSLIB_RDATA_WF_APL } },
  	/* 43 */
  	{ DNSLIB_RRTYPE_DS, "DS", 4,
  	  { DNSLIB_RDATA_WF_SHORT, DNSLIB_RDATA_WF_BYTE, DNSLIB_RDATA_WF_BYTE, DNSLIB_RDATA_WF_BINARY } },
  	/* 44 */
  	{ DNSLIB_RRTYPE_SSHFP, "SSHFP", 3,
  	  { DNSLIB_RDATA_WF_BYTE, DNSLIB_RDATA_WF_BYTE, DNSLIB_RDATA_WF_BINARY } },
  	/* 45 */
  	{ DNSLIB_RRTYPE_IPSECKEY, "IPSECKEY", 5,
  	  { DNSLIB_RDATA_WF_BYTE, DNSLIB_RDATA_WF_BYTE, DNSLIB_RDATA_WF_BYTE, DNSLIB_RDATA_WF_IPSECGATEWAY,
  	    DNSLIB_RDATA_WF_BINARY } },
  	/* 46 */
  	{ DNSLIB_RRTYPE_RRSIG, "RRSIG", 9,
  	  { DNSLIB_RDATA_WF_SHORT, DNSLIB_RDATA_WF_BYTE, DNSLIB_RDATA_WF_BYTE, DNSLIB_RDATA_WF_LONG,
  	    DNSLIB_RDATA_WF_LONG, DNSLIB_RDATA_WF_LONG, DNSLIB_RDATA_WF_SHORT,
  	    DNSLIB_RDATA_WF_LITERAL_DNAME, DNSLIB_RDATA_WF_BINARY } },
  	/* 47 */
  	{ DNSLIB_RRTYPE_NSEC, "NSEC", 2,
  	  { DNSLIB_RDATA_WF_LITERAL_DNAME, DNSLIB_RDATA_WF_BINARY } },
  	/* 48 */
  	{ DNSLIB_RRTYPE_DNSKEY, "DNSKEY", 4,
  	  { DNSLIB_RDATA_WF_SHORT, DNSLIB_RDATA_WF_BYTE, DNSLIB_RDATA_WF_BYTE, DNSLIB_RDATA_WF_BINARY } },
  	/* 49 */
  	{ DNSLIB_RRTYPE_DHCID, "DHCID", 1, { DNSLIB_RDATA_WF_BINARY } },
  	/* 50 */
  	{ DNSLIB_RRTYPE_NSEC3, "NSEC3", 6,
  	  { DNSLIB_RDATA_WF_BYTE, /* hash type */
  	    DNSLIB_RDATA_WF_BYTE, /* flags */
  	    DNSLIB_RDATA_WF_SHORT, /* iterations */
  	    DNSLIB_RDATA_WF_BINARYWITHLENGTH, /* salt */
  	    DNSLIB_RDATA_WF_BINARYWITHLENGTH, /* next hashed name */
  	    DNSLIB_RDATA_WF_BINARY /* type bitmap */ } },
  	/* 51 */
  	{ DNSLIB_RRTYPE_NSEC3PARAM, "NSEC3PARAM", 4,
  	  { DNSLIB_RDATA_WF_BYTE, /* hash type */
  	    DNSLIB_RDATA_WF_BYTE, /* flags */
  	    DNSLIB_RDATA_WF_SHORT, /* iterations */
  	    DNSLIB_RDATA_WF_BINARYWITHLENGTH /* salt */ } },
  	/* 52 */
  
  
    /* In NSD they have indices between 52 and 99 filled with
     unknown types. TODO add here if it's really needed? */
  
    /* There's a GNU extension that works like this: [first ... last] = value */
  
  	/* 99 */
  	[99] = { DNSLIB_RRTYPE_SPF, "SPF", MAXRDATALEN,
  	  { DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT,
  	    DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT,
  	    DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT,
  	    DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT,
  	    DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT,
  	    DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT,
  	    DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT,
  	    DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT,
  	    DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT,
  	    DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT,
  	    DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT,
  	    DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT,
  	    DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT,
  	    DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT,
  	    DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT,
  	    DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT, DNSLIB_RDATA_WF_TEXT } },
  	/* 32769 */
  /*	[100] = { DNSLIB_RRTYPE_DLV, "DLV", 4,
  	  { DNSLIB_RDATA_WF_SHORT, DNSLIB_RDATA_WF_BYTE, DNSLIB_RDATA_WF_BYTE, DNSLIB_RDATA_WF_BINARY } },*/
};

lookup_table_type *lookup_by_name( lookup_table_type *table, const char *name )
{
    while (table->name != NULL) {
        if (strcasecmp(name, table->name) == 0)
            return table;
    		table++;
	  }

	  return NULL;
}

lookup_table_type *lookup_by_id( lookup_table_type *table, int id )
{
    while (table->name != NULL) {
        if (table->id == id)
            return table;
        table++;
    }

    return NULL;
}

size_t strlcpy( char *dst, const char *src, size_t siz )
{
    char *d = dst;
    const char *s = src;
    size_t n = siz;

    /* Copy as many bytes as will fit */
    if (n != 0 && --n != 0) {
        do {
              if ((*d++ = *s++) == 0)
                  break;
        } while (--n != 0);
    }

    /* Not enough room in dst, add NUL and traverse rest of src */
    if (n == 0) {
        if (siz != 0)
            *d = '\0';                /* NUL-terminate dst */
        while (*s++)
             ;
    }

    return(s - src - 1);        /* count does not include NUL */
}

dnslib_rrtype_descriptor_t *dnslib_rrtype_descriptor_by_type( uint16_t type )
{
  	if (type < DNSLIB_RRTYPE_DESCRIPTORS_LENGTH)
  	  	return &dnslib_rrtype_descriptors[type];
  	else if (type == DNSLIB_RRTYPE_DLV)
  	  	return &dnslib_rrtype_descriptors[DNSLIB_RRTYPE_DS];
  	return &dnslib_rrtype_descriptors[0];
}

//Will we ever need this? XXX
dnslib_rrtype_descriptor_t *rrtype_descriptor_by_name( const char *name )
{
  	int i;
  
  	for (i = 0; i < DNSLIB_RRTYPE_DESCRIPTORS_LENGTH; ++i) {
  		if (dnslib_rrtype_descriptors[i].name
  		    && strcasecmp(dnslib_rrtype_descriptors[i].name, name) == 0)
  		{
  			return &dnslib_rrtype_descriptors[i];
  		}
  	}
  
  	if (dnslib_rrtype_descriptors[DNSLIB_RRTYPE_DLV].name
  	    && strcasecmp(dnslib_rrtype_descriptors[DNSLIB_RRTYPE_DLV].name, 
                      name) == 0)
  	{
  		return &dnslib_rrtype_descriptors[DNSLIB_RRTYPE_DS];
  	}
  
  	return NULL;
}

const char *rrtype_to_string( uint16_t rrtype )
{
  	static char buf[20];
  	dnslib_rrtype_descriptor_t *descriptor = 
    dnslib_rrtype_descriptor_by_type(rrtype);
  	if (descriptor->name) {
  		return descriptor->name;
  	} else {
  		snprintf(buf, sizeof(buf), "TYPE%d", (int) rrtype);
  		return buf;
  	}
}

uint16_t rrtype_from_string( const char *name )
{
    char *end;
    long rrtype;
  	dnslib_rrtype_descriptor_t *entry;
  
  	entry = rrtype_descriptor_by_name(name);
  	if (entry) {
  	  	return entry->type;
  	}
  
  	if (strlen(name) < 5)
  	  	return 0;
  
  	if (strncasecmp(name, "TYPE", 4) != 0)
  	  	return 0;
  
  	if (!isdigit((int)name[4]))
  	  	return 0;
  
  	/* The rest from the string must be a number.  */
  	rrtype = strtol(name + 4, &end, 10);
  	if (*end != '\0')
    		return 0;
  	if (rrtype < 0 || rrtype > 65535L)
  	  	return 0;
  
    return (uint16_t) rrtype;
}

const char *rrclass_to_string(uint16_t rrclass)
{
  	static char buf[20];
  	lookup_table_type *entry = lookup_by_id(dns_rrclasses, rrclass);
  	if (entry) {
  	  	assert(strlen(entry->name) < sizeof(buf));
        strlcpy(buf, entry->name, sizeof(buf)); 
  	} else {
  	  	snprintf(buf, sizeof(buf), "CLASS%d", (int) rrclass);
  	}
  	return buf;
}

uint16_t rrclass_from_string(const char *name)
{
    char *end;
    long rrclass;
  	lookup_table_type *entry;
  
  	entry = lookup_by_name(dns_rrclasses, name);
  	if (entry) {
  	  	return (uint16_t) entry->id;
  	}
  
  	if (strlen(name) < 6)
  		  return 0;
  
  	if (strncasecmp(name, "CLASS", 5) != 0)
  		  return 0;
  
  	if (!isdigit((int)name[5]))
  		  return 0;
  
  // The rest from the string must be a number. 
  	rrclass = strtol(name + 5, &end, 10);
  	if (*end != '\0')
  		  return 0;
  	if (rrclass < 0 || rrclass > 65535L)
  	  	return 0;
  
  	return (uint16_t) rrclass;
}

/* end of file descriptor.c */
