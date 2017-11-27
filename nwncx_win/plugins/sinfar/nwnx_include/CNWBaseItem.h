#ifndef _NX_NWN_STRUCT_CNWBASEITEM_
#define _NX_NWN_STRUCT_CNWBASEITEM_

struct CNWBaseItem_s {
	uint32_t	tlk_name;				/* 0x000 */
	uint32_t	bi_equip_slots;			/* 0x004 */
	uint8_t		WeaponWield;			/* 0x008 */
	uint8_t		WeaponType;				/* 0x009 */
	uint8_t		field_A;				/* 0x00A */
	uint8_t		field_B;				/* 0x00B */
	uint32_t	MinRange;				/* 0x00C */
	uint32_t	MaxRange;				/* 0x0010 */
	uint8_t		InvSlotWidth;			/* 0x0014 */
	uint8_t		InvSlotHeight;			/* 0x0015 */
	uint8_t		ModelType;				/* 0x0016 */
	uint8_t		ChargesStarting;		/* 0x0017 */
	uint8_t		RangedWeapon;			/* 0x0018 */
	uint8_t		WeaponSize;				/* 0x0019 */
	uint8_t		NumDice;				/* 0x001A */
	uint8_t		DieToRoll;				/* 0x001B */
	uint8_t		CritThreat;				/* 0x001C */
	uint8_t		CritHitMult;			/* 0x001D */
	uint8_t		Category;				/* 0x001E */
	uint8_t		field_1F;				/* 0x001F */
	float		BaseCost;				/* 0x0020 */
	uint8_t		Max_StackSize;			/* 0x0024 */
	uint8_t		field_25;				/* 0x0025 */
	uint8_t		field_26;				/* 0x0026 */
	uint8_t		field_27;				/* 0x0027 */
	uint8_t		ItemMultiplier;			/* 0x0028 */
	uint8_t		field_29;				/* 0x0029 */
	uint8_t		field_2A;				/* 0x002A */
	uint8_t		field_2B;				/* 0x002B */
	uint32_t	tlk_description;		/* 0x002C */
	uint8_t		MinProps;				/* 0x0030 */
	uint8_t		MaxProps;				/* 0x0031 */
	uint8_t		PropColumn;				/* 0x0032 */
	uint8_t		StorePanel;				/* 0x0033 */
	uint8_t		StorePanelSort;			/* 0x0034 */
	uint8_t		AnimSlashL;				/* 0x0035 */
	uint8_t		AnimSlashR;				/* 0x0036 */
	uint8_t		AnimSlashS;				/* 0x0037 */
	uint8_t		ILRStackSize;			/* 0x0038 */
	uint8_t		field_39;				/* 0x0039 */
	uint8_t		field_3A;				/* 0x003A */
	uint8_t		field_3B;				/* 0x003B */
	float		PrefAttackDist;			/* 0x003C */
	char		ItemClass[16];			/* 0x0040 */
	uint32_t	CanRotateIcon;			/* 0x0050 */
	uint32_t	field_54;				/* 0x0054 */
	char		DefaultIcon[16];		/* 0x0058 */
	char		DefaultModel[16];		/* 0x0068 */
	uint8_t		field_78;				/* 0x0078 */
	uint8_t		field_79;				/* 0x0079 */
	uint8_t		field_7A;				/* 0x007A */
	uint8_t		field_7B;				/* 0x007B */
	uint8_t		Container;				/* 0x007C */
	uint8_t		field_7D;				/* 0x007D */
	uint8_t		field_7E;				/* 0x007E */
	uint8_t		field_7F;				/* 0x007F */
	uint8_t		field_80;				/* 0x0080 */
	uint8_t		field_81;				/* 0x0081 */
	uint8_t		field_82;				/* 0x0082 */
	uint8_t		field_83;				/* 0x0083 */
	uint32_t	InventorySoundType;		/* 0x0084 */
	uint32_t	*pReqFeats;				/* 0x0088 */
	uint32_t	ReqFeatsNumber;			/* 0x008C */
	uint32_t	tlk_BaseItemStatRef;	/* 0x0090 */
	uint32_t	RotateOnGround;			/* 0x0094 */
	uint32_t	TenthLBS;				/* 0x0098 */
	uint8_t		Base_AC;				/* 0x009C */
	uint8_t		AC_Enchant;				/* 0x009D */
	uint8_t		WeaponMatType;			/* 0x009E */
	char		ArmorCheckPen;			/* 0x009F */
	uint8_t		AmmunitionType;			/* 0x00A0 */
	uint8_t		QBBehaviour;			/* 0x00A1 */
	uint8_t		ArcaneSpellFailure;		/* 0x00A2 */
	uint8_t		field_A3;				/* 0x00A3 */};

#endif /* _NX_NWN_STRUCT_CNWBASEITEM_ */

