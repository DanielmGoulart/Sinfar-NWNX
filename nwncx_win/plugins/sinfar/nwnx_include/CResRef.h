#ifndef _H_CRESREF_H_
#define _H_CRESREF_H_

struct CResRef_s {
	char value[16];
	
	CResRef *CResRef(CExoString const &Ref);
}; 

#endif;