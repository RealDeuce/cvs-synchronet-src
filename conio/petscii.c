#include "keys.h"

struct ascii_trans {
	unsigned char	native;
	unsigned char	cp437;
};

struct key_trans {
	int	keypress;
	int	translated;
};

struct ascii_trans display_petscii[] = {
	 {32,' '}
	,{33,'!'}
	,{34,'"'}
	,{35,'#'}
	,{36,'$'}
	,{37,'%'}
	,{38,'&'}
	,{39,'\''}
	,{40,'('}
	,{41,')'}
	,{42,'*'}
	,{43,'+'}
	,{44,','}
	,{45,'-'}
	,{46,'.'}
	,{47,'/'}
	,{48,'0'}
	,{49,'1'}
	,{50,'2'}
	,{51,'3'}
	,{52,'4'}
	,{53,'5'}
	,{54,'6'}
	,{55,'7'}
	,{56,'8'}
	,{57,'9'}
	,{58,':'}
	,{59,';'}
	,{60,'<'}
	,{61,'='}
	,{62,'>'}
	,{63,'?'}
	,{64,'@'}
	,{65,'a'}
	,{66,'b'}
	,{67,'c'}
	,{68,'d'}
	,{69,'e'}
	,{70,'f'}
	,{71,'g'}
	,{72,'h'}
	,{73,'i'}
	,{74,'j'}
	,{75,'k'}
	,{76,'l'}
	,{77,'m'}
	,{78,'n'}
	,{79,'o'}
	,{80,'p'}
	,{81,'q'}
	,{82,'r'}
	,{83,'s'}
	,{84,'t'}
	,{85,'u'}
	,{86,'v'}
	,{87,'w'}
	,{88,'x'}
	,{89,'y'}
	,{90,'z'}
	,{91,'['}
	,{92,156}
	,{93,']'}
	,{94,24}
	,{95,27}
	,{96,196}
	,{97,'A'}
	,{98,'B'}
	,{99,'C'}
	,{100,'D'}
	,{101,'E'}
	,{102,'F'}
	,{103,'G'}
	,{104,'H'}
	,{105,'I'}
	,{106,'J'}
	,{107,'K'}
	,{108,'L'}
	,{109,'M'}
	,{110,'N'}
	,{111,'O'}
	,{112,'P'}
	,{113,'Q'}
	,{114,'R'}
	,{115,'S'}
	,{116,'T'}
	,{117,'U'}
	,{118,'V'}
	,{119,'W'}
	,{120,'X'}
	,{121,'Y'}
	,{122,'Z'}
	,{123,197}
	,{124,181}	///* Left side fine 50% checker */
	,{125,179}
	,{126,178}	///* Course 50% checker */
	,{127,176}	///* Horizontal cross-hatch (light shade) */
	,{160,255}
	,{161,221}
	,{162,220}
	,{163,'^'}	/* Row 1 set */
	,{164,'_'}	///* Row 8 set */
	,{165,'{'}	/* Col 1 set */
	,{166,177}	///* Fine 50% checker */
	,{167,'}'}	/* Col 8 set */
	,{168,210}	///* Bottom fine 50% checker */
	,{169,31}	/* Top left half set */
	,{170,245}	/* Cols 7 & 8 set */
	,{171,195}
	,{172,201}	///* Bottom right quadrant set */
	,{173,192}
	,{174,191}
	,{175,205}	/* Rows 7 & 8 set */
	,{176,218}
	,{177,193}
	,{178,194}
	,{179,180}
	,{180,244}	/* Cols 1 & 2 set */
	,{181,185}	/* Cols 1, 2, & 3 set */
	,{182,222}	/* Cols 6, 7, & 8 set */
	,{183,169}	/* Rows 1 & 2 set */
	,{184,223}	/* Rows 1, 2, & 3 set */
	,{185,22}	/* Rows 6, 7, & 8 set */
	,{186,251}
	,{187,187}	///* Bottom left quadrant set */
	,{188,200}	///* Top right quadrant set */
	,{189,217}
	,{190,188}	///* Top left quadrant set */
	,{191,206}	/* Top left and bottom right quadrants set */
};

struct key_trans input_petscii[] = {
	 {'A', 'a'}
	,{'B', 'b'}
	,{'C', 'c'}
	,{'D', 'd'}
	,{'E', 'e'}
	,{'F', 'f'}
	,{'G', 'g'}
	,{'H', 'h'}
	,{'I', 'i'}
	,{'J', 'j'}
	,{'K', 'k'}
	,{'L', 'l'}
	,{'M', 'm'}
	,{'N', 'n'}
	,{'O', 'o'}
	,{'P', 'p'}
	,{'Q', 'q'}
	,{'R', 'r'}
	,{'S', 's'}
	,{'T', 't'}
	,{'U', 'u'}
	,{'V', 'v'}
	,{'W', 'w'}
	,{'X', 'x'}
	,{'Y', 'y'}
	,{'Z', 'z'}
	,{'a', 'A'}
	,{'b', 'B'}
	,{'c', 'C'}
	,{'d', 'D'}
	,{'e', 'E'}
	,{'f', 'F'}
	,{'g', 'G'}
	,{'h', 'H'}
	,{'i', 'I'}
	,{'j', 'J'}
	,{'k', 'K'}
	,{'l', 'L'}
	,{'m', 'M'}
	,{'n', 'N'}
	,{'o', 'O'}
	,{'p', 'P'}
	,{'q', 'Q'}
	,{'r', 'R'}
	,{'s', 'S'}
	,{'t', 'T'}
	,{'u', 'U'}
	,{'v', 'V'}
	,{'w', 'W'}
	,{'x', 'X'}
	,{'y', 'Y'}
	,{'z', 'Z'}
	,{'\n',141}
	,{CIO_KEY_DOWN, 17}
	,{CIO_KEY_HOME, 19}
	,{CIO_KEY_DC, 20}
	,{'\b', 20}
	,{CIO_KEY_RIGHT, 29}
	,{CIO_KEY_F(1), 133}
	,{CIO_KEY_F(3), 134}
	,{CIO_KEY_F(5), 135}
	,{CIO_KEY_F(7), 136}
	,{CIO_KEY_F(2), 137}
	,{CIO_KEY_F(4), 138}
	,{CIO_KEY_F(6), 139}
	,{CIO_KEY_F(8), 140}
	,{CIO_KEY_UP, 145}
	,{CIO_KEY_IC, 148}
	,{CIO_KEY_LEFT, 157}
	,{CIO_KEY_END, 147}
	,{CIO_KEY_PPAGE, 131}
	,{CIO_KEY_NPAGE, 3}
};

