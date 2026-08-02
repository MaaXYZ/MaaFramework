#pragma once

#include <string>

static constexpr std::string_view kAsciiKeyMap =
    "xkb_keymap {\n"
    "xkb_keycodes \"ascii\" {\n"
    "	minimum = 8;\n"
    "	maximum = 127;\n"
    "\n"
    "	<BKSP> = 8;\n"
    "	<TAB>  = 9;\n"
    "	<RTRN> = 10;\n"
    "	<ESC>  = 27;\n"
    "	<SPCE> = 32;\n"
    "	<EXCL> = 33;\n"
    "	<QDBL> = 34;\n"
    "	<HASH> = 35;\n"
    "	<DOLL> = 36;\n"
    "	<PCNT> = 37;\n"
    "	<AMP>  = 38;\n"
    "	<APOS> = 39;\n"
    "	<LPAR> = 40;\n"
    "	<RPAR> = 41;\n"
    "	<STAR> = 42;\n"
    "	<PLUS> = 43;\n"
    "	<COMM> = 44;\n"
    "	<MIN>  = 45;\n"
    "	<PRD>  = 46;\n"
    "	<FWSL> = 47;\n"
    "	<NUM0> = 48;\n"
    "	<NUM1> = 49;\n"
    "	<NUM2> = 50;\n"
    "	<NUM3> = 51;\n"
    "	<NUM4> = 52;\n"
    "	<NUM5> = 53;\n"
    "	<NUM6> = 54;\n"
    "	<NUM7> = 55;\n"
    "	<NUM8> = 56;\n"
    "	<NUM9> = 57;\n"
    "	<COLN> = 58;\n"
    "	<SCLN> = 59;\n"
    "	<LESS> = 60;\n"
    "	<EQL>  = 61;\n"
    "	<GRTR> = 62;\n"
    "	<QUES> = 63;\n"
    "	<ATSN> = 64;\n"
    "	<KEYA> = 65;\n"
    "	<KEYB> = 66;\n"
    "	<KEYC> = 67;\n"
    "	<KEYD> = 68;\n"
    "	<KEYE> = 69;\n"
    "	<KEYF> = 70;\n"
    "	<KEYG> = 71;\n"
    "	<KEYH> = 72;\n"
    "	<KEYI> = 73;\n"
    "	<KEYJ> = 74;\n"
    "	<KEYK> = 75;\n"
    "	<KEYL> = 76;\n"
    "	<KEYM> = 77;\n"
    "	<KEYN> = 78;\n"
    "	<KEYO> = 79;\n"
    "	<KEYP> = 80;\n"
    "	<KEYQ> = 81;\n"
    "	<KEYR> = 82;\n"
    "	<KEYS> = 83;\n"
    "	<KEYT> = 84;\n"
    "	<KEYU> = 85;\n"
    "	<KEYV> = 86;\n"
    "	<KEYW> = 87;\n"
    "	<KEYX> = 88;\n"
    "	<KEYY> = 89;\n"
    "	<KEYZ> = 90;\n"
    "	<LBRA> = 91;\n"
    "	<BKSL> = 92;\n"
    "	<RBRA> = 93;\n"
    "	<CRCM> = 94;\n"
    "	<UNDR> = 95;\n"
    "	<GRV>  = 96;\n"
    "	<KEYa> = 97;\n"
    "	<KEYb> = 98;\n"
    "	<KEYc> = 99;\n"
    "	<KEYd> = 100;\n"
    "	<KEYe> = 101;\n"
    "	<KEYf> = 102;\n"
    "	<KEYg> = 103;\n"
    "	<KEYh> = 104;\n"
    "	<KEYi> = 105;\n"
    "	<KEYj> = 106;\n"
    "	<KEYk> = 107;\n"
    "	<KEYl> = 108;\n"
    "	<KEYm> = 109;\n"
    "	<KEYn> = 110;\n"
    "	<KEYo> = 111;\n"
    "	<KEYp> = 112;\n"
    "	<KEYq> = 113;\n"
    "	<KEYr> = 114;\n"
    "	<KEYs> = 115;\n"
    "	<KEYt> = 116;\n"
    "	<KEYu> = 117;\n"
    "	<KEYv> = 118;\n"
    "	<KEYw> = 119;\n"
    "	<KEYx> = 120;\n"
    "	<KEYy> = 121;\n"
    "	<KEYz> = 122;\n"
    "	<LBRC> = 123;\n"
    "	<BAR>  = 124;\n"
    "	<RBRC> = 125;\n"
    "	<TLDE> = 126;\n"
    "};\n"
    "\n"
    "xkb_types \"flat\" {\n"
    "	virtual_modifiers NumLock;\n"
    "\n"
    "	type \"ONE_LEVEL\" {\n"
    "		modifiers =none;\n"
    "		level_name[1] = \"Any\";\n"
    "	};\n"
    "	type \"TWO_LEVEL\" {\n"
    "		modifiers = Shift;\n"
    "		map[Shift] = Level2;\n"
    "		level_name[Level1] = \"Base\";\n"
    "		level_name[Level2] = \"Shift\";\n"
    "	};\n"
    "	type \"ALPHABETIC\" {\n"
    "		modifiers = Shift+Lock;\n"
    "		map[Shift] = Level2;\n"
    "		map[Lock] = Level2;\n"
    "		level_name[Level1] = \"Base\";\n"
    "		level_name[Level2] = \"Caps\";\n"
    "	};\n"
    "	type \"KEYPAD\" {\n"
    "		modifiers = Shift+NumLock;\n"
    "		map[None] = Level1;\n"
    "		map[Shift] = Level2;\n"
    "		map[NumLock] = Level2;\n"
    "		map[Shift+NumLock] = Level1;\n"
    "		level_name[Level1] = \"Base\";\n"
    "		level_name[Level2] = \"Number\";\n"
    "	};\n"
    "};\n"
    "\n"
    "xkb_compatibility \"basic\" {\n"
    "    virtual_modifiers NumLock;\n"
    "\n"
    "    interpret.useModMapMods = AnyLevel;\n"
    "    interpret.repeat = False;\n"
    "    interpret.locking = False;\n"
    "    interpret Shift_Lock+AnyOf(Shift+Lock) {\n"
    "        action = LockMods(modifiers=Shift);\n"
    "    };\n"
    "};\n"
    "\n"
    "xkb_symbols \"ascii\" {\n"
    "	name[Group1]=\"ASCII RAW\";\n"
    "\n"
    "	key <BKSP> {[ BackSpace ]};\n"
    "	key <TAB>  {[ Tab ]};\n"
    "	key <RTRN> {[ Return ]};\n"
    "	key <ESC>  {[ Escape ]};\n"
    "	key <SPCE> {[ space ]};\n"
    "	key <EXCL> {[ exclam ]};\n"
    "	key <QDBL> {[ quotedbl ]};\n"
    "	key <HASH> {[ numbersign ]};\n"
    "	key <DOLL> {[ dollar ]};\n"
    "	key <PCNT> {[ percent ]};\n"
    "	key <AMP>  {[ ampersand ]};\n"
    "	key <APOS> {[ apostrophe ]};\n"
    "	key <LPAR> {[ parenleft ]};\n"
    "	key <RPAR> {[ parenright ]};\n"
    "	key <STAR> {[ asterisk ]};\n"
    "	key <PLUS> {[ plus ]};\n"
    "	key <COMM> {[ comma ]};\n"
    "	key <MIN>  {[ minus ]};\n"
    "	key <PRD>  {[ period ]};\n"
    "	key <FWSL> {[ slash ]};\n"
    "	key <NUM0> {[ 0 ]};\n"
    "	key <NUM1> {[ 1 ]};\n"
    "	key <NUM2> {[ 2 ]};\n"
    "	key <NUM3> {[ 3 ]};\n"
    "	key <NUM4> {[ 4 ]};\n"
    "	key <NUM5> {[ 5 ]};\n"
    "	key <NUM6> {[ 6 ]};\n"
    "	key <NUM7> {[ 7 ]};\n"
    "	key <NUM8> {[ 8 ]};\n"
    "	key <NUM9> {[ 9 ]};\n"
    "	key <COLN> {[ colon ]};\n"
    "	key <SCLN> {[ semicolon ]};\n"
    "	key <LESS> {[ less ]};\n"
    "	key <EQL>  {[ equal ]};\n"
    "	key <GRTR> {[ greater ]};\n"
    "	key <QUES> {[ question ]};\n"
    "	key <ATSN> {[ at ]};\n"
    "	key <KEYA> {[ A ]};\n"
    "	key <KEYB> {[ B ]};\n"
    "	key <KEYC> {[ C ]};\n"
    "	key <KEYD> {[ D ]};\n"
    "	key <KEYE> {[ E ]};\n"
    "	key <KEYF> {[ F ]};\n"
    "	key <KEYG> {[ G ]};\n"
    "	key <KEYH> {[ H ]};\n"
    "	key <KEYI> {[ I ]};\n"
    "	key <KEYJ> {[ J ]};\n"
    "	key <KEYK> {[ K ]};\n"
    "	key <KEYL> {[ L ]};\n"
    "	key <KEYM> {[ M ]};\n"
    "	key <KEYN> {[ N ]};\n"
    "	key <KEYO> {[ O ]};\n"
    "	key <KEYP> {[ P ]};\n"
    "	key <KEYQ> {[ Q ]};\n"
    "	key <KEYR> {[ R ]};\n"
    "	key <KEYS> {[ S ]};\n"
    "	key <KEYT> {[ T ]};\n"
    "	key <KEYU> {[ U ]};\n"
    "	key <KEYV> {[ V ]};\n"
    "	key <KEYW> {[ W ]};\n"
    "	key <KEYX> {[ X ]};\n"
    "	key <KEYY> {[ Y ]};\n"
    "	key <KEYZ> {[ Z ]};\n"
    "	key <LBRA> {[ bracketleft ]};\n"
    "	key <BKSL> {[ backslash ]};\n"
    "	key <RBRA> {[ bracketright ]};\n"
    "	key <CRCM> {[ asciicircum ]};\n"
    "	key <UNDR> {[ underscore ]};\n"
    "	key <GRV>  {[ grave ]};\n"
    "	key <KEYa> {[ a ]};\n"
    "	key <KEYb> {[ b ]};\n"
    "	key <KEYc> {[ c ]};\n"
    "	key <KEYd> {[ d ]};\n"
    "	key <KEYe> {[ e ]};\n"
    "	key <KEYf> {[ f ]};\n"
    "	key <KEYg> {[ g ]};\n"
    "	key <KEYh> {[ h ]};\n"
    "	key <KEYi> {[ i ]};\n"
    "	key <KEYj> {[ j ]};\n"
    "	key <KEYk> {[ k ]};\n"
    "	key <KEYl> {[ l ]};\n"
    "	key <KEYm> {[ m ]};\n"
    "	key <KEYn> {[ n ]};\n"
    "	key <KEYo> {[ o ]};\n"
    "	key <KEYp> {[ p ]};\n"
    "	key <KEYq> {[ q ]};\n"
    "	key <KEYr> {[ r ]};\n"
    "	key <KEYs> {[ s ]};\n"
    "	key <KEYt> {[ t ]};\n"
    "	key <KEYu> {[ u ]};\n"
    "	key <KEYv> {[ v ]};\n"
    "	key <KEYw> {[ w ]};\n"
    "	key <KEYx> {[ x ]};\n"
    "	key <KEYy> {[ y ]};\n"
    "	key <KEYz> {[ z ]};\n"
    "	key <LBRC> {[ braceleft ]};\n"
    "	key <BAR>  {[ bar ]};\n"
    "	key <RBRC> {[ braceright ]};\n"
    "	key <TLDE> {[ asciitilde ]};\n"
    "};\n"
    "};\n";

static constexpr std::string_view kScanCodeKeyMap =
    "xkb_keymap {\
xkb_keycodes \"(unnamed)\" {\
        minimum = 8;\
        maximum = 255;\
        <ESC>                = 9;\
        <AE01>               = 10;\
        <AE02>               = 11;\
        <AE03>               = 12;\
        <AE04>               = 13;\
        <AE05>               = 14;\
        <AE06>               = 15;\
        <AE07>               = 16;\
        <AE08>               = 17;\
        <AE09>               = 18;\
        <AE10>               = 19;\
        <AE11>               = 20;\
        <AE12>               = 21;\
        <BKSP>               = 22;\
        <TAB>                = 23;\
        <AD01>               = 24;\
        <AD02>               = 25;\
        <AD03>               = 26;\
        <AD04>               = 27;\
        <AD05>               = 28;\
        <AD06>               = 29;\
        <AD07>               = 30;\
        <AD08>               = 31;\
        <AD09>               = 32;\
        <AD10>               = 33;\
        <AD11>               = 34;\
        <AD12>               = 35;\
        <RTRN>               = 36;\
        <LCTL>               = 37;\
        <AC01>               = 38;\
        <AC02>               = 39;\
        <AC03>               = 40;\
        <AC04>               = 41;\
        <AC05>               = 42;\
        <AC06>               = 43;\
        <AC07>               = 44;\
        <AC08>               = 45;\
        <AC09>               = 46;\
        <AC10>               = 47;\
        <AC11>               = 48;\
        <TLDE>               = 49;\
        <LFSH>               = 50;\
        <BKSL>               = 51;\
        <AB01>               = 52;\
        <AB02>               = 53;\
        <AB03>               = 54;\
        <AB04>               = 55;\
        <AB05>               = 56;\
        <AB06>               = 57;\
        <AB07>               = 58;\
        <AB08>               = 59;\
        <AB09>               = 60;\
        <AB10>               = 61;\
        <RTSH>               = 62;\
        <KPMU>               = 63;\
        <LALT>               = 64;\
        <SPCE>               = 65;\
        <CAPS>               = 66;\
        <FK01>               = 67;\
        <FK02>               = 68;\
        <FK03>               = 69;\
        <FK04>               = 70;\
        <FK05>               = 71;\
        <FK06>               = 72;\
        <FK07>               = 73;\
        <FK08>               = 74;\
        <FK09>               = 75;\
        <FK10>               = 76;\
        <NMLK>               = 77;\
        <SCLK>               = 78;\
        <KP7>                = 79;\
        <KP8>                = 80;\
        <KP9>                = 81;\
        <KPSU>               = 82;\
        <KP4>                = 83;\
        <KP5>                = 84;\
        <KP6>                = 85;\
        <KPAD>               = 86;\
        <KP1>                = 87;\
        <KP2>                = 88;\
        <KP3>                = 89;\
        <KP0>                = 90;\
        <KPDL>               = 91;\
        <LVL3>               = 92;\
        <LSGT>               = 94;\
        <FK11>               = 95;\
        <FK12>               = 96;\
        <AB11>               = 97;\
        <KATA>               = 98;\
        <HIRA>               = 99;\
        <HENK>               = 100;\
        <HKTG>               = 101;\
        <MUHE>               = 102;\
        <JPCM>               = 103;\
        <KPEN>               = 104;\
        <RCTL>               = 105;\
        <KPDV>               = 106;\
        <PRSC>               = 107;\
        <RALT>               = 108;\
        <LNFD>               = 109;\
        <HOME>               = 110;\
        <UP>                 = 111;\
        <PGUP>               = 112;\
        <LEFT>               = 113;\
        <RGHT>               = 114;\
        <END>                = 115;\
        <DOWN>               = 116;\
        <PGDN>               = 117;\
        <INS>                = 118;\
        <DELE>               = 119;\
        <I120>               = 120;\
        <MUTE>               = 121;\
        <VOL->               = 122;\
        <VOL+>               = 123;\
        <POWR>               = 124;\
        <KPEQ>               = 125;\
        <I126>               = 126;\
        <PAUS>               = 127;\
        <I128>               = 128;\
        <I129>               = 129;\
        <HNGL>               = 130;\
        <HJCV>               = 131;\
        <AE13>               = 132;\
        <LWIN>               = 133;\
        <RWIN>               = 134;\
        <COMP>               = 135;\
        <STOP>               = 136;\
        <AGAI>               = 137;\
        <PROP>               = 138;\
        <UNDO>               = 139;\
        <FRNT>               = 140;\
        <COPY>               = 141;\
        <OPEN>               = 142;\
        <PAST>               = 143;\
        <FIND>               = 144;\
        <CUT>                = 145;\
        <HELP>               = 146;\
        <I147>               = 147;\
        <I148>               = 148;\
        <I149>               = 149;\
        <I150>               = 150;\
        <I151>               = 151;\
        <I152>               = 152;\
        <I153>               = 153;\
        <I154>               = 154;\
        <I155>               = 155;\
        <I156>               = 156;\
        <I157>               = 157;\
        <I158>               = 158;\
        <I159>               = 159;\
        <I160>               = 160;\
        <I161>               = 161;\
        <I162>               = 162;\
        <I163>               = 163;\
        <I164>               = 164;\
        <I165>               = 165;\
        <I166>               = 166;\
        <I167>               = 167;\
        <I168>               = 168;\
        <I169>               = 169;\
        <I170>               = 170;\
        <I171>               = 171;\
        <I172>               = 172;\
        <I173>               = 173;\
        <I174>               = 174;\
        <I175>               = 175;\
        <I176>               = 176;\
        <I177>               = 177;\
        <I178>               = 178;\
        <I179>               = 179;\
        <I180>               = 180;\
        <I181>               = 181;\
        <I182>               = 182;\
        <I183>               = 183;\
        <I184>               = 184;\
        <I185>               = 185;\
        <I186>               = 186;\
        <I187>               = 187;\
        <I188>               = 188;\
        <I189>               = 189;\
        <I190>               = 190;\
        <FK13>               = 191;\
        <FK14>               = 192;\
        <FK15>               = 193;\
        <FK16>               = 194;\
        <FK17>               = 195;\
        <FK18>               = 196;\
        <FK19>               = 197;\
        <FK20>               = 198;\
        <FK21>               = 199;\
        <FK22>               = 200;\
        <FK23>               = 201;\
        <FK24>               = 202;\
        <MDSW>               = 203;\
        <ALT>                = 204;\
        <META>               = 205;\
        <SUPR>               = 206;\
        <HYPR>               = 207;\
        <I208>               = 208;\
        <I209>               = 209;\
        <I210>               = 210;\
        <I211>               = 211;\
        <I212>               = 212;\
        <I213>               = 213;\
        <I214>               = 214;\
        <I215>               = 215;\
        <I216>               = 216;\
        <I217>               = 217;\
        <I218>               = 218;\
        <I219>               = 219;\
        <I220>               = 220;\
        <I221>               = 221;\
        <I222>               = 222;\
        <I223>               = 223;\
        <I224>               = 224;\
        <I225>               = 225;\
        <I226>               = 226;\
        <I227>               = 227;\
        <I228>               = 228;\
        <I229>               = 229;\
        <I230>               = 230;\
        <I231>               = 231;\
        <I232>               = 232;\
        <I233>               = 233;\
        <I234>               = 234;\
        <I235>               = 235;\
        <I236>               = 236;\
        <I237>               = 237;\
        <I238>               = 238;\
        <I239>               = 239;\
        <I240>               = 240;\
        <I241>               = 241;\
        <I242>               = 242;\
        <I243>               = 243;\
        <I244>               = 244;\
        <I245>               = 245;\
        <I246>               = 246;\
        <I247>               = 247;\
        <I248>               = 248;\
        <I249>               = 249;\
        <I250>               = 250;\
        <I251>               = 251;\
        <I252>               = 252;\
        <I253>               = 253;\
        <I254>               = 254;\
        <I255>               = 255;\
        indicator 1 = \"Caps Lock\";\
        indicator 2 = \"Num Lock\";\
        indicator 3 = \"Scroll Lock\";\
        indicator 4 = \"Compose\";\
        indicator 5 = \"Kana\";\
        indicator 6 = \"Sleep\";\
        indicator 7 = \"Suspend\";\
        indicator 8 = \"Mute\";\
        indicator 9 = \"Misc\";\
        indicator 10 = \"Mail\";\
        indicator 11 = \"Charging\";\
        indicator 12 = \"Shift Lock\";\
        indicator 13 = \"Group 2\";\
        indicator 14 = \"Mouse Keys\";\
        alias <AC12>         = <BKSL>;\
        alias <MENU>         = <COMP>;\
        alias <HZTG>         = <TLDE>;\
        alias <LMTA>         = <LWIN>;\
        alias <RMTA>         = <RWIN>;\
        alias <ALGR>         = <RALT>;\
        alias <KPPT>         = <I129>;\
        alias <LatQ>         = <AD01>;\
        alias <LatW>         = <AD02>;\
        alias <LatE>         = <AD03>;\
        alias <LatR>         = <AD04>;\
        alias <LatT>         = <AD05>;\
        alias <LatY>         = <AD06>;\
        alias <LatU>         = <AD07>;\
        alias <LatI>         = <AD08>;\
        alias <LatO>         = <AD09>;\
        alias <LatP>         = <AD10>;\
        alias <LatA>         = <AC01>;\
        alias <LatS>         = <AC02>;\
        alias <LatD>         = <AC03>;\
        alias <LatF>         = <AC04>;\
        alias <LatG>         = <AC05>;\
        alias <LatH>         = <AC06>;\
        alias <LatJ>         = <AC07>;\
        alias <LatK>         = <AC08>;\
        alias <LatL>         = <AC09>;\
        alias <LatZ>         = <AB01>;\
        alias <LatX>         = <AB02>;\
        alias <LatC>         = <AB03>;\
        alias <LatV>         = <AB04>;\
        alias <LatB>         = <AB05>;\
        alias <LatN>         = <AB06>;\
        alias <LatM>         = <AB07>;\
};\
\
xkb_types \"(unnamed)\" {\
        virtual_modifiers NumLock,Alt,LevelThree,LAlt,RAlt,RControl,LControl,ScrollLock,LevelFive,AltGr,Meta,Super,Hyper;\
\
        type \"ONE_LEVEL\" {\
                modifiers= none;\
                level_name[Level1]= \"Any\";\
        };\
        type \"TWO_LEVEL\" {\
                modifiers= Shift;\
                map[Shift]= Level2;\
                level_name[Level1]= \"Base\";\
                level_name[Level2]= \"Shift\";\
        };\
        type \"ALPHABETIC\" {\
                modifiers= Shift+Lock;\
                map[Shift]= Level2;\
                map[Lock]= Level2;\
                level_name[Level1]= \"Base\";\
                level_name[Level2]= \"Caps\";\
        };\
        type \"SHIFT+ALT\" {\
                modifiers= Shift+Alt;\
                map[Shift+Alt]= Level2;\
                level_name[Level1]= \"Base\";\
                level_name[Level2]= \"Shift+Alt\";\
        };\
        type \"PC_SUPER_LEVEL2\" {\
                modifiers= Mod4;\
                map[Mod4]= Level2;\
                level_name[Level1]= \"Base\";\
                level_name[Level2]= \"Super\";\
        };\
        type \"PC_CONTROL_LEVEL2\" {\
                modifiers= Control;\
                map[Control]= Level2;\
                level_name[Level1]= \"Base\";\
                level_name[Level2]= \"Control\";\
        };\
        type \"PC_LCONTROL_LEVEL2\" {\
                modifiers= LControl;\
                map[LControl]= Level2;\
                level_name[Level1]= \"Base\";\
                level_name[Level2]= \"LControl\";\
        };\
        type \"PC_RCONTROL_LEVEL2\" {\
                modifiers= RControl;\
                map[RControl]= Level2;\
                level_name[Level1]= \"Base\";\
                level_name[Level2]= \"RControl\";\
        };\
        type \"PC_ALT_LEVEL2\" {\
                modifiers= Alt;\
                map[Alt]= Level2;\
                level_name[Level1]= \"Base\";\
                level_name[Level2]= \"Alt\";\
        };\
        type \"PC_LALT_LEVEL2\" {\
                modifiers= LAlt;\
                map[LAlt]= Level2;\
                level_name[Level1]= \"Base\";\
                level_name[Level2]= \"LAlt\";\
        };\
        type \"PC_RALT_LEVEL2\" {\
                modifiers= RAlt;\
                map[RAlt]= Level2;\
                level_name[Level1]= \"Base\";\
                level_name[Level2]= \"RAlt\";\
        };\
        type \"CTRL+ALT\" {\
                modifiers= Shift+Control+Alt+LevelThree;\
                map[Shift]= Level2;\
                preserve[Shift]= Shift;\
                map[LevelThree]= Level3;\
                map[Shift+LevelThree]= Level4;\
                preserve[Shift+LevelThree]= Shift;\
                map[Control+Alt]= Level5;\
                level_name[Level1]= \"Base\";\
                level_name[Level2]= \"Shift\";\
                level_name[Level3]= \"Alt Base\";\
                level_name[Level4]= \"Shift Alt\";\
                level_name[Level5]= \"Ctrl+Alt\";\
        };\
        type \"LOCAL_EIGHT_LEVEL\" {\
                modifiers= Shift+Lock+Control+LevelThree;\
                map[Shift]= Level2;\
                map[Lock]= Level2;\
                map[LevelThree]= Level3;\
                map[Shift+Lock+LevelThree]= Level3;\
                map[Shift+LevelThree]= Level4;\
                map[Lock+LevelThree]= Level4;\
                map[Control]= Level5;\
                map[Shift+Lock+Control]= Level5;\
                map[Shift+Control]= Level6;\
                map[Lock+Control]= Level6;\
                map[Control+LevelThree]= Level7;\
                map[Shift+Lock+Control+LevelThree]= Level7;\
                map[Shift+Control+LevelThree]= Level8;\
                map[Lock+Control+LevelThree]= Level8;\
                level_name[Level1]= \"Base\";\
                level_name[Level2]= \"Shift\";\
                level_name[Level3]= \"Level3\";\
                level_name[Level4]= \"Shift Level3\";\
                level_name[Level5]= \"Ctrl\";\
                level_name[Level6]= \"Shift Ctrl\";\
                level_name[Level7]= \"Level3 Ctrl\";\
                level_name[Level8]= \"Shift Level3 Ctrl\";\
        };\
        type \"THREE_LEVEL\" {\
                modifiers= Shift+LevelThree;\
                map[Shift]= Level2;\
                map[LevelThree]= Level3;\
                map[Shift+LevelThree]= Level3;\
                level_name[Level1]= \"Base\";\
                level_name[Level2]= \"Shift\";\
                level_name[Level3]= \"Level3\";\
        };\
        type \"EIGHT_LEVEL\" {\
                modifiers= Shift+LevelThree+LevelFive;\
                map[Shift]= Level2;\
                map[LevelThree]= Level3;\
                map[Shift+LevelThree]= Level4;\
                map[LevelFive]= Level5;\
                map[Shift+LevelFive]= Level6;\
                map[LevelThree+LevelFive]= Level7;\
                map[Shift+LevelThree+LevelFive]= Level8;\
                level_name[Level1]= \"Base\";\
                level_name[Level2]= \"Shift\";\
                level_name[Level3]= \"Alt Base\";\
                level_name[Level4]= \"Shift Alt\";\
                level_name[Level5]= \"X\";\
                level_name[Level6]= \"X Shift\";\
                level_name[Level7]= \"X Alt Base\";\
                level_name[Level8]= \"X Shift Alt\";\
        };\
        type \"EIGHT_LEVEL_ALPHABETIC\" {\
                modifiers= Shift+Lock+LevelThree+LevelFive;\
                map[Shift]= Level2;\
                map[Lock]= Level2;\
                map[LevelThree]= Level3;\
                map[Shift+LevelThree]= Level4;\
                map[Lock+LevelThree]= Level4;\
                map[Shift+Lock+LevelThree]= Level3;\
                map[LevelFive]= Level5;\
                map[Shift+LevelFive]= Level6;\
                map[Lock+LevelFive]= Level6;\
                map[LevelThree+LevelFive]= Level7;\
                map[Shift+LevelThree+LevelFive]= Level8;\
                map[Lock+LevelThree+LevelFive]= Level8;\
                map[Shift+Lock+LevelThree+LevelFive]= Level7;\
                level_name[Level1]= \"Base\";\
                level_name[Level2]= \"Shift\";\
                level_name[Level3]= \"Alt Base\";\
                level_name[Level4]= \"Shift Alt\";\
                level_name[Level5]= \"X\";\
                level_name[Level6]= \"X Shift\";\
                level_name[Level7]= \"X Alt Base\";\
                level_name[Level8]= \"X Shift Alt\";\
        };\
        type \"EIGHT_LEVEL_LEVEL_FIVE_LOCK\" {\
                modifiers= Shift+Lock+NumLock+LevelThree+LevelFive;\
                map[Shift]= Level2;\
                map[LevelThree]= Level3;\
                map[Shift+LevelThree]= Level4;\
                map[LevelFive]= Level5;\
                map[Shift+LevelFive]= Level6;\
                preserve[Shift+LevelFive]= Shift;\
                map[LevelThree+LevelFive]= Level7;\
                map[Shift+LevelThree+LevelFive]= Level8;\
                map[NumLock]= Level5;\
                map[Shift+NumLock]= Level6;\
                preserve[Shift+NumLock]= Shift;\
                map[NumLock+LevelThree]= Level7;\
                map[Shift+NumLock+LevelThree]= Level8;\
                map[Shift+NumLock+LevelFive]= Level2;\
                map[NumLock+LevelThree+LevelFive]= Level3;\
                map[Shift+NumLock+LevelThree+LevelFive]= Level4;\
                map[Shift+Lock]= Level2;\
                map[Lock+LevelThree]= Level3;\
                map[Shift+Lock+LevelThree]= Level4;\
                map[Lock+LevelFive]= Level5;\
                map[Shift+Lock+LevelFive]= Level6;\
                preserve[Shift+Lock+LevelFive]= Shift;\
                map[Lock+LevelThree+LevelFive]= Level7;\
                map[Shift+Lock+LevelThree+LevelFive]= Level8;\
                map[Lock+NumLock]= Level5;\
                map[Shift+Lock+NumLock]= Level6;\
                preserve[Shift+Lock+NumLock]= Shift;\
                map[Lock+NumLock+LevelThree]= Level7;\
                map[Shift+Lock+NumLock+LevelThree]= Level8;\
                map[Shift+Lock+NumLock+LevelFive]= Level2;\
                map[Lock+NumLock+LevelThree+LevelFive]= Level3;\
                map[Shift+Lock+NumLock+LevelThree+LevelFive]= Level4;\
                level_name[Level1]= \"Base\";\
                level_name[Level2]= \"Shift\";\
                level_name[Level3]= \"Alt Base\";\
                level_name[Level4]= \"Shift Alt\";\
                level_name[Level5]= \"X\";\
                level_name[Level6]= \"X Shift\";\
                level_name[Level7]= \"X Alt Base\";\
                level_name[Level8]= \"X Shift Alt\";\
        };\
        type \"EIGHT_LEVEL_ALPHABETIC_LEVEL_FIVE_LOCK\" {\
                modifiers= Shift+Lock+NumLock+LevelThree+LevelFive;\
                map[Shift]= Level2;\
                map[LevelThree]= Level3;\
                map[Shift+LevelThree]= Level4;\
                map[LevelFive]= Level5;\
                map[Shift+LevelFive]= Level6;\
                preserve[Shift+LevelFive]= Shift;\
                map[LevelThree+LevelFive]= Level7;\
                map[Shift+LevelThree+LevelFive]= Level8;\
                map[NumLock]= Level5;\
                map[Shift+NumLock]= Level6;\
                preserve[Shift+NumLock]= Shift;\
                map[NumLock+LevelThree]= Level7;\
                map[Shift+NumLock+LevelThree]= Level8;\
                map[Shift+NumLock+LevelFive]= Level2;\
                map[NumLock+LevelThree+LevelFive]= Level3;\
                map[Shift+NumLock+LevelThree+LevelFive]= Level4;\
                map[Lock]= Level2;\
                map[Lock+LevelThree]= Level3;\
                map[Shift+Lock+LevelThree]= Level4;\
                map[Lock+LevelFive]= Level5;\
                map[Shift+Lock+LevelFive]= Level6;\
                map[Lock+LevelThree+LevelFive]= Level7;\
                map[Shift+Lock+LevelThree+LevelFive]= Level8;\
                map[Lock+NumLock]= Level5;\
                map[Shift+Lock+NumLock]= Level6;\
                map[Lock+NumLock+LevelThree]= Level7;\
                map[Shift+Lock+NumLock+LevelThree]= Level8;\
                map[Lock+NumLock+LevelFive]= Level2;\
                map[Lock+NumLock+LevelThree+LevelFive]= Level4;\
                map[Shift+Lock+NumLock+LevelThree+LevelFive]= Level3;\
                level_name[Level1]= \"Base\";\
                level_name[Level2]= \"Shift\";\
                level_name[Level3]= \"Alt Base\";\
                level_name[Level4]= \"Shift Alt\";\
                level_name[Level5]= \"X\";\
                level_name[Level6]= \"X Shift\";\
                level_name[Level7]= \"X Alt Base\";\
                level_name[Level8]= \"X Shift Alt\";\
        };\
        type \"EIGHT_LEVEL_SEMIALPHABETIC\" {\
                modifiers= Shift+Lock+LevelThree+LevelFive;\
                map[Shift]= Level2;\
                map[Lock]= Level2;\
                map[LevelThree]= Level3;\
                map[Shift+LevelThree]= Level4;\
                map[Lock+LevelThree]= Level3;\
                preserve[Lock+LevelThree]= Lock;\
                map[Shift+Lock+LevelThree]= Level4;\
                preserve[Shift+Lock+LevelThree]= Lock;\
                map[LevelFive]= Level5;\
                map[Shift+LevelFive]= Level6;\
                map[Lock+LevelFive]= Level6;\
                preserve[Lock+LevelFive]= Lock;\
                map[Shift+Lock+LevelFive]= Level6;\
                preserve[Shift+Lock+LevelFive]= Lock;\
                map[LevelThree+LevelFive]= Level7;\
                map[Shift+LevelThree+LevelFive]= Level8;\
                map[Lock+LevelThree+LevelFive]= Level7;\
                preserve[Lock+LevelThree+LevelFive]= Lock;\
                map[Shift+Lock+LevelThree+LevelFive]= Level8;\
                preserve[Shift+Lock+LevelThree+LevelFive]= Lock;\
                level_name[Level1]= \"Base\";\
                level_name[Level2]= \"Shift\";\
                level_name[Level3]= \"Alt Base\";\
                level_name[Level4]= \"Shift Alt\";\
                level_name[Level5]= \"X\";\
                level_name[Level6]= \"X Shift\";\
                level_name[Level7]= \"X Alt Base\";\
                level_name[Level8]= \"X Shift Alt\";\
        };\
        type \"FOUR_LEVEL\" {\
                modifiers= Shift+LevelThree;\
                map[Shift]= Level2;\
                map[LevelThree]= Level3;\
                map[Shift+LevelThree]= Level4;\
                level_name[Level1]= \"Base\";\
                level_name[Level2]= \"Shift\";\
                level_name[Level3]= \"Alt Base\";\
                level_name[Level4]= \"Shift Alt\";\
        };\
        type \"FOUR_LEVEL_ALPHABETIC\" {\
                modifiers= Shift+Lock+LevelThree;\
                map[Shift]= Level2;\
                map[Lock]= Level2;\
                map[LevelThree]= Level3;\
                map[Shift+LevelThree]= Level4;\
                map[Lock+LevelThree]= Level4;\
                map[Shift+Lock+LevelThree]= Level3;\
                level_name[Level1]= \"Base\";\
                level_name[Level2]= \"Shift\";\
                level_name[Level3]= \"Alt Base\";\
                level_name[Level4]= \"Shift Alt\";\
        };\
        type \"FOUR_LEVEL_SEMIALPHABETIC\" {\
                modifiers= Shift+Lock+LevelThree;\
                map[Shift]= Level2;\
                map[Lock]= Level2;\
                map[LevelThree]= Level3;\
                map[Shift+LevelThree]= Level4;\
                map[Lock+LevelThree]= Level3;\
                preserve[Lock+LevelThree]= Lock;\
                map[Shift+Lock+LevelThree]= Level4;\
                preserve[Shift+Lock+LevelThree]= Lock;\
                level_name[Level1]= \"Base\";\
                level_name[Level2]= \"Shift\";\
                level_name[Level3]= \"Alt Base\";\
                level_name[Level4]= \"Shift Alt\";\
        };\
        type \"FOUR_LEVEL_MIXED_KEYPAD\" {\
                modifiers= Shift+NumLock+LevelThree;\
                map[NumLock]= Level2;\
                map[Shift]= Level2;\
                map[LevelThree]= Level3;\
                map[NumLock+LevelThree]= Level3;\
                map[Shift+LevelThree]= Level4;\
                map[Shift+NumLock+LevelThree]= Level4;\
                level_name[Level1]= \"Base\";\
                level_name[Level2]= \"Number\";\
                level_name[Level3]= \"Alt Base\";\
                level_name[Level4]= \"Shift Alt\";\
        };\
        type \"FOUR_LEVEL_X\" {\
                modifiers= Shift+Control+Alt+LevelThree;\
                map[LevelThree]= Level2;\
                map[Shift+LevelThree]= Level3;\
                map[Control+Alt]= Level4;\
                level_name[Level1]= \"Base\";\
                level_name[Level2]= \"Alt Base\";\
                level_name[Level3]= \"Shift Alt\";\
                level_name[Level4]= \"Ctrl+Alt\";\
        };\
        type \"SEPARATE_CAPS_AND_SHIFT_ALPHABETIC\" {\
                modifiers= Shift+Lock+LevelThree;\
                map[Shift]= Level2;\
                map[Lock]= Level4;\
                preserve[Lock]= Lock;\
                map[LevelThree]= Level3;\
                map[Shift+LevelThree]= Level4;\
                map[Lock+LevelThree]= Level3;\
                preserve[Lock+LevelThree]= Lock;\
                map[Shift+Lock+LevelThree]= Level3;\
                level_name[Level1]= \"Base\";\
                level_name[Level2]= \"Shift\";\
                level_name[Level3]= \"AltGr Base\";\
                level_name[Level4]= \"Shift AltGr\";\
        };\
        type \"FOUR_LEVEL_PLUS_LOCK\" {\
                modifiers= Shift+Lock+LevelThree;\
                map[Shift]= Level2;\
                map[LevelThree]= Level3;\
                map[Shift+LevelThree]= Level4;\
                map[Lock]= Level5;\
                map[Shift+Lock]= Level2;\
                map[Lock+LevelThree]= Level3;\
                map[Shift+Lock+LevelThree]= Level4;\
                level_name[Level1]= \"Base\";\
                level_name[Level2]= \"Shift\";\
                level_name[Level3]= \"Alt Base\";\
                level_name[Level4]= \"Shift Alt\";\
                level_name[Level5]= \"Lock\";\
        };\
        type \"KEYPAD\" {\
                modifiers= Shift+NumLock;\
                map[Shift]= Level2;\
                map[NumLock]= Level2;\
                level_name[Level1]= \"Base\";\
                level_name[Level2]= \"Number\";\
        };\
        type \"FOUR_LEVEL_KEYPAD\" {\
                modifiers= Shift+NumLock+LevelThree;\
                map[Shift]= Level2;\
                map[NumLock]= Level2;\
                map[LevelThree]= Level3;\
                map[Shift+LevelThree]= Level4;\
                map[NumLock+LevelThree]= Level4;\
                map[Shift+NumLock+LevelThree]= Level3;\
                level_name[Level1]= \"Base\";\
                level_name[Level2]= \"Number\";\
                level_name[Level3]= \"Alt Base\";\
                level_name[Level4]= \"Alt Number\";\
        };\
};\
\
xkb_compatibility \"(unnamed)\" {\
        virtual_modifiers NumLock,Alt,LevelThree,LAlt,RAlt,RControl,LControl,ScrollLock,LevelFive,AltGr,Meta,Super,Hyper;\
\
        interpret.useModMapMods= AnyLevel;\
        interpret.repeat= False;\
        interpret ISO_Level2_Latch+Exactly(Shift) {\
                useModMapMods=level1;\
                action= LatchMods(modifiers=Shift,clearLocks,latchToLock);\
        };\
        interpret Shift_Lock+AnyOf(Shift+Lock) {\
                action= LockMods(modifiers=Shift);\
        };\
        interpret Num_Lock+AnyOf(all) {\
                virtualModifier= NumLock;\
                action= LockMods(modifiers=NumLock);\
        };\
        interpret ISO_Level3_Shift+AnyOf(all) {\
                virtualModifier= LevelThree;\
                useModMapMods=level1;\
                action= SetMods(modifiers=LevelThree,clearLocks);\
        };\
        interpret ISO_Level3_Latch+AnyOf(all) {\
                virtualModifier= LevelThree;\
                useModMapMods=level1;\
                action= LatchMods(modifiers=LevelThree,clearLocks,latchToLock);\
        };\
        interpret ISO_Level3_Lock+AnyOf(all) {\
                virtualModifier= LevelThree;\
                useModMapMods=level1;\
                action= LockMods(modifiers=LevelThree);\
        };\
        interpret Alt_L+AnyOf(all) {\
                virtualModifier= Alt;\
                action= SetMods(modifiers=modMapMods,clearLocks);\
        };\
        interpret Alt_R+AnyOf(all) {\
                virtualModifier= Alt;\
                action= SetMods(modifiers=modMapMods,clearLocks);\
        };\
        interpret Meta_L+AnyOf(all) {\
                virtualModifier= Meta;\
                action= SetMods(modifiers=modMapMods,clearLocks);\
        };\
        interpret Meta_R+AnyOf(all) {\
                virtualModifier= Meta;\
                action= SetMods(modifiers=modMapMods,clearLocks);\
        };\
        interpret Super_L+AnyOf(all) {\
                virtualModifier= Super;\
                action= SetMods(modifiers=modMapMods,clearLocks);\
        };\
        interpret Super_R+AnyOf(all) {\
                virtualModifier= Super;\
                action= SetMods(modifiers=modMapMods,clearLocks);\
        };\
        interpret Hyper_L+AnyOf(all) {\
                virtualModifier= Hyper;\
                action= SetMods(modifiers=modMapMods,clearLocks);\
        };\
        interpret Hyper_R+AnyOf(all) {\
                virtualModifier= Hyper;\
                action= SetMods(modifiers=modMapMods,clearLocks);\
        };\
        interpret Scroll_Lock+AnyOf(all) {\
                virtualModifier= ScrollLock;\
                action= LockMods(modifiers=modMapMods);\
        };\
        interpret ISO_Level5_Shift+AnyOf(all) {\
                virtualModifier= LevelFive;\
                useModMapMods=level1;\
                action= SetMods(modifiers=LevelFive,clearLocks);\
        };\
        interpret ISO_Level5_Latch+AnyOf(all) {\
                virtualModifier= LevelFive;\
                useModMapMods=level1;\
                action= LatchMods(modifiers=LevelFive,clearLocks,latchToLock);\
        };\
        interpret ISO_Level5_Lock+AnyOf(all) {\
                virtualModifier= LevelFive;\
                useModMapMods=level1;\
                action= LockMods(modifiers=LevelFive);\
        };\
        interpret Mode_switch+AnyOfOrNone(all) {\
                virtualModifier= AltGr;\
                useModMapMods=level1;\
                action= SetGroup(group=+1);\
        };\
        interpret ISO_Level3_Shift+AnyOfOrNone(all) {\
                action= SetMods(modifiers=LevelThree,clearLocks);\
        };\
        interpret ISO_Level3_Latch+AnyOfOrNone(all) {\
                action= LatchMods(modifiers=LevelThree,clearLocks,latchToLock);\
        };\
        interpret ISO_Level3_Lock+AnyOfOrNone(all) {\
                action= LockMods(modifiers=LevelThree);\
        };\
        interpret ISO_Group_Latch+AnyOfOrNone(all) {\
                virtualModifier= AltGr;\
                useModMapMods=level1;\
                action= LatchGroup(group=2);\
        };\
        interpret ISO_Next_Group+AnyOfOrNone(all) {\
                virtualModifier= AltGr;\
                useModMapMods=level1;\
                action= LockGroup(group=+1);\
        };\
        interpret ISO_Prev_Group+AnyOfOrNone(all) {\
                virtualModifier= AltGr;\
                useModMapMods=level1;\
                action= LockGroup(group=-1);\
        };\
        interpret ISO_First_Group+AnyOfOrNone(all) {\
                action= LockGroup(group=1);\
        };\
        interpret ISO_Last_Group+AnyOfOrNone(all) {\
                action= LockGroup(group=2);\
        };\
        interpret KP_1+AnyOfOrNone(all) {\
                repeat= True;\
                action= MovePtr(x=-1,y=+1);\
        };\
        interpret KP_End+AnyOfOrNone(all) {\
                repeat= True;\
                action= MovePtr(x=-1,y=+1);\
        };\
        interpret KP_2+AnyOfOrNone(all) {\
                repeat= True;\
                action= MovePtr(x=+0,y=+1);\
        };\
        interpret KP_Down+AnyOfOrNone(all) {\
                repeat= True;\
                action= MovePtr(x=+0,y=+1);\
        };\
        interpret KP_3+AnyOfOrNone(all) {\
                repeat= True;\
                action= MovePtr(x=+1,y=+1);\
        };\
        interpret KP_Next+AnyOfOrNone(all) {\
                repeat= True;\
                action= MovePtr(x=+1,y=+1);\
        };\
        interpret KP_4+AnyOfOrNone(all) {\
                repeat= True;\
                action= MovePtr(x=-1,y=+0);\
        };\
        interpret KP_Left+AnyOfOrNone(all) {\
                repeat= True;\
                action= MovePtr(x=-1,y=+0);\
        };\
        interpret KP_6+AnyOfOrNone(all) {\
                repeat= True;\
                action= MovePtr(x=+1,y=+0);\
        };\
        interpret KP_Right+AnyOfOrNone(all) {\
                repeat= True;\
                action= MovePtr(x=+1,y=+0);\
        };\
        interpret KP_7+AnyOfOrNone(all) {\
                repeat= True;\
                action= MovePtr(x=-1,y=-1);\
        };\
        interpret KP_Home+AnyOfOrNone(all) {\
                repeat= True;\
                action= MovePtr(x=-1,y=-1);\
        };\
        interpret KP_8+AnyOfOrNone(all) {\
                repeat= True;\
                action= MovePtr(x=+0,y=-1);\
        };\
        interpret KP_Up+AnyOfOrNone(all) {\
                repeat= True;\
                action= MovePtr(x=+0,y=-1);\
        };\
        interpret KP_9+AnyOfOrNone(all) {\
                repeat= True;\
                action= MovePtr(x=+1,y=-1);\
        };\
        interpret KP_Prior+AnyOfOrNone(all) {\
                repeat= True;\
                action= MovePtr(x=+1,y=-1);\
        };\
        interpret KP_5+AnyOfOrNone(all) {\
                repeat= True;\
                action= PtrBtn(button=default);\
        };\
        interpret KP_Begin+AnyOfOrNone(all) {\
                repeat= True;\
                action= PtrBtn(button=default);\
        };\
        interpret KP_F2+AnyOfOrNone(all) {\
                repeat= True;\
                action= SetPtrDflt(affect=button,button=1);\
        };\
        interpret KP_Divide+AnyOfOrNone(all) {\
                repeat= True;\
                action= SetPtrDflt(affect=button,button=1);\
        };\
        interpret KP_F3+AnyOfOrNone(all) {\
                repeat= True;\
                action= SetPtrDflt(affect=button,button=2);\
        };\
        interpret KP_Multiply+AnyOfOrNone(all) {\
                repeat= True;\
                action= SetPtrDflt(affect=button,button=2);\
        };\
        interpret KP_F4+AnyOfOrNone(all) {\
                repeat= True;\
                action= SetPtrDflt(affect=button,button=3);\
        };\
        interpret KP_Subtract+AnyOfOrNone(all) {\
                repeat= True;\
                action= SetPtrDflt(affect=button,button=3);\
        };\
        interpret KP_Separator+AnyOfOrNone(all) {\
                repeat= True;\
                action= PtrBtn(button=default,count=2);\
        };\
        interpret KP_Add+AnyOfOrNone(all) {\
                repeat= True;\
                action= PtrBtn(button=default,count=2);\
        };\
        interpret KP_0+AnyOfOrNone(all) {\
                repeat= True;\
                action= LockPtrBtn(button=default,affect=lock);\
        };\
        interpret KP_Insert+AnyOfOrNone(all) {\
                repeat= True;\
                action= LockPtrBtn(button=default,affect=lock);\
        };\
        interpret KP_Decimal+AnyOfOrNone(all) {\
                repeat= True;\
                action= LockPtrBtn(button=default,affect=unlock);\
        };\
        interpret KP_Delete+AnyOfOrNone(all) {\
                repeat= True;\
                action= LockPtrBtn(button=default,affect=unlock);\
        };\
        interpret F25+AnyOfOrNone(all) {\
                repeat= True;\
                action= SetPtrDflt(affect=button,button=1);\
        };\
        interpret F26+AnyOfOrNone(all) {\
                repeat= True;\
                action= SetPtrDflt(affect=button,button=2);\
        };\
        interpret F27+AnyOfOrNone(all) {\
                repeat= True;\
                action= MovePtr(x=-1,y=-1);\
        };\
        interpret F29+AnyOfOrNone(all) {\
                repeat= True;\
                action= MovePtr(x=+1,y=-1);\
        };\
        interpret F31+AnyOfOrNone(all) {\
                repeat= True;\
                action= PtrBtn(button=default);\
        };\
        interpret F33+AnyOfOrNone(all) {\
                repeat= True;\
                action= MovePtr(x=-1,y=+1);\
        };\
        interpret F35+AnyOfOrNone(all) {\
                repeat= True;\
                action= MovePtr(x=+1,y=+1);\
        };\
        interpret Pointer_Button_Dflt+AnyOfOrNone(all) {\
                action= PtrBtn(button=default);\
        };\
        interpret Pointer_Button1+AnyOfOrNone(all) {\
                action= PtrBtn(button=1);\
        };\
        interpret Pointer_Button2+AnyOfOrNone(all) {\
                action= PtrBtn(button=2);\
        };\
        interpret Pointer_Button3+AnyOfOrNone(all) {\
                action= PtrBtn(button=3);\
        };\
        interpret Pointer_DblClick_Dflt+AnyOfOrNone(all) {\
                action= PtrBtn(button=default,count=2);\
        };\
        interpret Pointer_DblClick1+AnyOfOrNone(all) {\
                action= PtrBtn(button=1,count=2);\
        };\
        interpret Pointer_DblClick2+AnyOfOrNone(all) {\
                action= PtrBtn(button=2,count=2);\
        };\
        interpret Pointer_DblClick3+AnyOfOrNone(all) {\
                action= PtrBtn(button=3,count=2);\
        };\
        interpret Pointer_Drag_Dflt+AnyOfOrNone(all) {\
                action= LockPtrBtn(button=default);\
        };\
        interpret Pointer_Drag1+AnyOfOrNone(all) {\
                action= LockPtrBtn(button=1);\
        };\
        interpret Pointer_Drag2+AnyOfOrNone(all) {\
                action= LockPtrBtn(button=2);\
        };\
        interpret Pointer_Drag3+AnyOfOrNone(all) {\
                action= LockPtrBtn(button=3);\
        };\
        interpret Pointer_EnableKeys+AnyOfOrNone(all) {\
                action= LockControls(controls=MouseKeys);\
        };\
        interpret Pointer_Accelerate+AnyOfOrNone(all) {\
                action= LockControls(controls=MouseKeysAccel);\
        };\
        interpret Pointer_DfltBtnNext+AnyOfOrNone(all) {\
                action= SetPtrDflt(affect=button,button=+1);\
        };\
        interpret Pointer_DfltBtnPrev+AnyOfOrNone(all) {\
                action= SetPtrDflt(affect=button,button=-1);\
        };\
        interpret AccessX_Enable+AnyOfOrNone(all) {\
                action= LockControls(controls=AccessXKeys);\
        };\
        interpret AccessX_Feedback_Enable+AnyOfOrNone(all) {\
                action= LockControls(controls=AccessXFeedback);\
        };\
        interpret RepeatKeys_Enable+AnyOfOrNone(all) {\
                action= LockControls(controls=RepeatKeys);\
        };\
        interpret SlowKeys_Enable+AnyOfOrNone(all) {\
                action= LockControls(controls=SlowKeys);\
        };\
        interpret BounceKeys_Enable+AnyOfOrNone(all) {\
                action= LockControls(controls=BounceKeys);\
        };\
        interpret StickyKeys_Enable+AnyOfOrNone(all) {\
                action= LockControls(controls=StickyKeys);\
        };\
        interpret MouseKeys_Enable+AnyOfOrNone(all) {\
                action= LockControls(controls=MouseKeys);\
        };\
        interpret MouseKeys_Accel_Enable+AnyOfOrNone(all) {\
                action= LockControls(controls=MouseKeysAccel);\
        };\
        interpret Overlay1_Enable+AnyOfOrNone(all) {\
                action= LockControls(controls=none);\
        };\
        interpret Overlay2_Enable+AnyOfOrNone(all) {\
                action= LockControls(controls=none);\
        };\
        interpret AudibleBell_Enable+AnyOfOrNone(all) {\
                action= LockControls(controls=AudibleBell);\
        };\
        interpret Terminate_Server+AnyOfOrNone(all) {\
                action= Terminate();\
        };\
        interpret Alt_L+AnyOfOrNone(all) {\
                action= SetMods(modifiers=Alt,clearLocks);\
        };\
        interpret Alt_R+AnyOfOrNone(all) {\
                action= SetMods(modifiers=Alt,clearLocks);\
        };\
        interpret Meta_L+AnyOfOrNone(all) {\
                action= SetMods(modifiers=Meta,clearLocks);\
        };\
        interpret Meta_R+AnyOfOrNone(all) {\
                action= SetMods(modifiers=Meta,clearLocks);\
        };\
        interpret Super_L+AnyOfOrNone(all) {\
                action= SetMods(modifiers=Super,clearLocks);\
        };\
        interpret Super_R+AnyOfOrNone(all) {\
                action= SetMods(modifiers=Super,clearLocks);\
        };\
        interpret Hyper_L+AnyOfOrNone(all) {\
                action= SetMods(modifiers=Hyper,clearLocks);\
        };\
        interpret Hyper_R+AnyOfOrNone(all) {\
                action= SetMods(modifiers=Hyper,clearLocks);\
        };\
        interpret Shift_L+AnyOfOrNone(all) {\
                action= SetMods(modifiers=Shift,clearLocks);\
        };\
        interpret XF86Switch_VT_1+AnyOfOrNone(all) {\
                repeat= True;\
                action= SwitchScreen(screen=1,!same);\
        };\
        interpret XF86Switch_VT_2+AnyOfOrNone(all) {\
                repeat= True;\
                action= SwitchScreen(screen=2,!same);\
        };\
        interpret XF86Switch_VT_3+AnyOfOrNone(all) {\
                repeat= True;\
                action= SwitchScreen(screen=3,!same);\
        };\
        interpret XF86Switch_VT_4+AnyOfOrNone(all) {\
                repeat= True;\
                action= SwitchScreen(screen=4,!same);\
        };\
        interpret XF86Switch_VT_5+AnyOfOrNone(all) {\
                repeat= True;\
                action= SwitchScreen(screen=5,!same);\
        };\
        interpret XF86Switch_VT_6+AnyOfOrNone(all) {\
                repeat= True;\
                action= SwitchScreen(screen=6,!same);\
        };\
        interpret XF86Switch_VT_7+AnyOfOrNone(all) {\
                repeat= True;\
                action= SwitchScreen(screen=7,!same);\
        };\
        interpret XF86Switch_VT_8+AnyOfOrNone(all) {\
                repeat= True;\
                action= SwitchScreen(screen=8,!same);\
        };\
        interpret XF86Switch_VT_9+AnyOfOrNone(all) {\
                repeat= True;\
                action= SwitchScreen(screen=9,!same);\
        };\
        interpret XF86Switch_VT_10+AnyOfOrNone(all) {\
                repeat= True;\
                action= SwitchScreen(screen=10,!same);\
        };\
        interpret XF86Switch_VT_11+AnyOfOrNone(all) {\
                repeat= True;\
                action= SwitchScreen(screen=11,!same);\
        };\
        interpret XF86Switch_VT_12+AnyOfOrNone(all) {\
                repeat= True;\
                action= SwitchScreen(screen=12,!same);\
        };\
        interpret XF86LogGrabInfo+AnyOfOrNone(all) {\
                repeat= True;\
                action= Private(type=0x86,data[0]=0x50,data[1]=0x72,data[2]=0x47,data[3]=0x72,data[4]=0x62,data[5]=0x73,data[6]=0x00);\
        };\
        interpret XF86LogWindowTree+AnyOfOrNone(all) {\
                repeat= True;\
                action= Private(type=0x86,data[0]=0x50,data[1]=0x72,data[2]=0x57,data[3]=0x69,data[4]=0x6e,data[5]=0x73,data[6]=0x00);\
        };\
        interpret XF86Next_VMode+AnyOfOrNone(all) {\
                repeat= True;\
                action= Private(type=0x86,data[0]=0x2b,data[1]=0x56,data[2]=0x4d,data[3]=0x6f,data[4]=0x64,data[5]=0x65,data[6]=0x00);\
        };\
        interpret XF86Prev_VMode+AnyOfOrNone(all) {\
                repeat= True;\
                action= Private(type=0x86,data[0]=0x2d,data[1]=0x56,data[2]=0x4d,data[3]=0x6f,data[4]=0x64,data[5]=0x65,data[6]=0x00);\
        };\
        interpret ISO_Level5_Shift+AnyOfOrNone(all) {\
                action= SetMods(modifiers=LevelFive,clearLocks);\
        };\
        interpret ISO_Level5_Latch+AnyOfOrNone(all) {\
                action= LatchMods(modifiers=LevelFive,clearLocks,latchToLock);\
        };\
        interpret ISO_Level5_Lock+AnyOfOrNone(all) {\
                action= LockMods(modifiers=LevelFive);\
        };\
        interpret Caps_Lock+AnyOfOrNone(all) {\
                action= LockMods(modifiers=Lock);\
        };\
        interpret Any+Exactly(Lock) {\
                action= LockMods(modifiers=Lock);\
        };\
        interpret Any+AnyOf(all) {\
                action= SetMods(modifiers=modMapMods,clearLocks);\
        };\
        indicator \"Caps Lock\" {\
                whichModState= locked;\
                modifiers= Lock;\
        };\
        indicator \"Num Lock\" {\
                whichModState= locked;\
                modifiers= NumLock;\
        };\
        indicator \"Scroll Lock\" {\
                whichModState= locked;\
                modifiers= ScrollLock;\
        };\
        indicator \"Shift Lock\" {\
                whichModState= locked;\
                modifiers= Shift;\
        };\
        indicator \"Group 2\" {\
                groups= 0xfe;\
        };\
        indicator \"Mouse Keys\" {\
                controls= MouseKeys;\
        };\
};\
\
xkb_symbols \"(unnamed)\" {\
        name[group1]=\"English (US)\";\
\
        key <ESC>                {	[          Escape ] };\
        key <AE01>               {	[               1,          exclam ] };\
        key <AE02>               {	[               2,              at ] };\
        key <AE03>               {	[               3,      numbersign ] };\
        key <AE04>               {	[               4,          dollar ] };\
        key <AE05>               {	[               5,         percent ] };\
        key <AE06>               {	[               6,     asciicircum ] };\
        key <AE07>               {	[               7,       ampersand ] };\
        key <AE08>               {	[               8,        asterisk ] };\
        key <AE09>               {	[               9,       parenleft ] };\
        key <AE10>               {	[               0,      parenright ] };\
        key <AE11>               {	[           minus,      underscore ] };\
        key <AE12>               {	[           equal,            plus ] };\
        key <BKSP>               {	[       BackSpace,       BackSpace ] };\
        key <TAB>                {	[             Tab,    ISO_Left_Tab ] };\
        key <AD01>               {	[               q,               Q, 1 ] };\
        key <AD02>               {	[               w,               W, 2 ] };\
        key <AD03>               {	[               e,               E, 3 ] };\
        key <AD04>               {	[               r,               R, 4 ] };\
        key <AD05>               {	[               t,               T, 5 ] };\
        key <AD06>               {	[               y,               Y, 6 ] };\
        key <AD07>               {	[               u,               U, 7 ] };\
        key <AD08>               {	[               i,               I, 8 ] };\
        key <AD09>               {	[               o,               O, 9 ] };\
        key <AD10>               {	[               p,               P, 0 ] };\
        key <AD11>               {	[     bracketleft,       braceleft ] };\
        key <AD12>               {	[    bracketright,      braceright ] };\
        key <RTRN>               {	[          Return ] };\
        key <LCTL>               {	[       Control_L ] };\
        key <AC01>               {	[               a,               A, minus ] };\
        key <AC02>               {	[               s,               S, at ] };\
        key <AC03>               {	[               d,               D, asterisk ] };\
        key <AC04>               {	[               f,               F, asciicircum ] };\
        key <AC05>               {	[               g,               G, colon ] };\
        key <AC06>               {	[               h,               H, semicolon ] };\
        key <AC07>               {	[               j,               J, parenleft ] };\
        key <AC08>               {	[               k,               K, parenright ] };\
        key <AC09>               {	[               l,               L, asciitilde ] };\
        key <AC10>               {	[       semicolon,           colon ] };\
        key <AC11>               {	[      apostrophe,        quotedbl ] };\
        key <TLDE>               {	[           grave,      asciitilde ] };\
        key <LFSH>               {	[         Shift_L ] };\
        key <BKSL>               {	[       backslash,             bar ] };\
        key <AB01>               {	[               z,               Z, slash ] };\
        key <AB02>               {	[               x,               X, apostrophe ] };\
        key <AB03>               {	[               c,               C, quotedbl ] };\
        key <AB04>               {	[               v,               V, plus ] };\
        key <AB05>               {	[               b,               B, equal ] };\
        key <AB06>               {	[               n,               N, question ] };\
        key <AB07>               {	[               m,               M, exclam ] };\
        key <AB08>               {	[           comma,            less, backslash] };\
        key <AB09>               {	[          period,         greater, bar ] };\
        key <AB10>               {	[           slash,        question ] };\
        key <RTSH>               {	[         Shift_R ] };\
        key <KPMU>               {\
                type= \"CTRL+ALT\",\
                symbols[Group1]= [     KP_Multiply,     KP_Multiply,     KP_Multiply,     KP_Multiply,   XF86ClearGrab ]\
        };\
        key <LALT>               {	[           Alt_L,          Meta_L ] };\
        key <SPCE>               {	[           space ] };\
        key <CAPS>               {	[       Caps_Lock ] };\
        key <FK01>               {\
                type= \"CTRL+ALT\",\
                symbols[Group1]= [              F1,              F1,              F1,              F1, XF86Switch_VT_1 ]\
        };\
        key <FK02>               {\
                type= \"CTRL+ALT\",\
                symbols[Group1]= [              F2,              F2,              F2,              F2, XF86Switch_VT_2 ]\
        };\
        key <FK03>               {\
                type= \"CTRL+ALT\",\
                symbols[Group1]= [              F3,              F3,              F3,              F3, XF86Switch_VT_3 ]\
        };\
        key <FK04>               {\
                type= \"CTRL+ALT\",\
                symbols[Group1]= [              F4,              F4,              F4,              F4, XF86Switch_VT_4 ]\
        };\
        key <FK05>               {\
                type= \"CTRL+ALT\",\
                symbols[Group1]= [              F5,              F5,              F5,              F5, XF86Switch_VT_5 ]\
        };\
        key <FK06>               {\
                type= \"CTRL+ALT\",\
                symbols[Group1]= [              F6,              F6,              F6,              F6, XF86Switch_VT_6 ]\
        };\
        key <FK07>               {\
                type= \"CTRL+ALT\",\
                symbols[Group1]= [              F7,              F7,              F7,              F7, XF86Switch_VT_7 ]\
        };\
        key <FK08>               {\
                type= \"CTRL+ALT\",\
                symbols[Group1]= [              F8,              F8,              F8,              F8, XF86Switch_VT_8 ]\
        };\
        key <FK09>               {\
                type= \"CTRL+ALT\",\
                symbols[Group1]= [              F9,              F9,              F9,              F9, XF86Switch_VT_9 ]\
        };\
        key <FK10>               {\
                type= \"CTRL+ALT\",\
                symbols[Group1]= [             F10,             F10,             F10,             F10, XF86Switch_VT_10 ]\
        };\
        key <NMLK>               {	[        Num_Lock ] };\
        key <SCLK>               {	[     Scroll_Lock ] };\
        key <KP7>                {	[         KP_Home,            KP_7 ] };\
        key <KP8>                {	[           KP_Up,            KP_8 ] };\
        key <KP9>                {	[        KP_Prior,            KP_9 ] };\
        key <KPSU>               {\
                type= \"CTRL+ALT\",\
                symbols[Group1]= [     KP_Subtract,     KP_Subtract,     KP_Subtract,     KP_Subtract,  XF86Prev_VMode ]\
        };\
        key <KP4>                {	[         KP_Left,            KP_4 ] };\
        key <KP5>                {	[        KP_Begin,            KP_5 ] };\
        key <KP6>                {	[        KP_Right,            KP_6 ] };\
        key <KPAD>               {\
                type= \"CTRL+ALT\",\
                symbols[Group1]= [          KP_Add,          KP_Add,          KP_Add,          KP_Add,  XF86Next_VMode ]\
        };\
        key <KP1>                {	[          KP_End,            KP_1 ] };\
        key <KP2>                {	[         KP_Down,            KP_2 ] };\
        key <KP3>                {	[         KP_Next,            KP_3 ] };\
        key <KP0>                {	[       KP_Insert,            KP_0 ] };\
        key <KPDL>               {	[       KP_Delete,      KP_Decimal ] };\
        key <LVL3>               {	[ ISO_Level3_Shift ] };\
        key <LSGT>               {	[            less,         greater,             bar,       brokenbar ] };\
        key <FK11>               {\
                type= \"CTRL+ALT\",\
                symbols[Group1]= [             F11,             F11,             F11,             F11, XF86Switch_VT_11 ]\
        };\
        key <FK12>               {\
                type= \"CTRL+ALT\",\
                symbols[Group1]= [             F12,             F12,             F12,             F12, XF86Switch_VT_12 ]\
        };\
        key <KATA>               {	[        Katakana ] };\
        key <HIRA>               {	[        Hiragana ] };\
        key <HENK>               {	[     Henkan_Mode ] };\
        key <HKTG>               {	[ Hiragana_Katakana ] };\
        key <MUHE>               {	[        Muhenkan ] };\
        key <KPEN>               {	[        KP_Enter ] };\
        key <RCTL>               {	[       Control_R ] };\
        key <KPDV>               {\
                type= \"CTRL+ALT\",\
                symbols[Group1]= [       KP_Divide,       KP_Divide,       KP_Divide,       KP_Divide,      XF86Ungrab ]\
        };\
        key <PRSC>               {\
                type= \"PC_ALT_LEVEL2\",\
                symbols[Group1]= [           Print,         Sys_Req ]\
        };\
        key <RALT>               {\
                type= \"TWO_LEVEL\",\
                symbols[Group1]= [           Alt_R,          Meta_R ]\
        };\
        key <LNFD>               {	[        Linefeed ] };\
        key <HOME>               {	[            Home ] };\
        key <UP>                 {	[              Up ] };\
        key <PGUP>               {	[           Prior ] };\
        key <LEFT>               {	[            Left ] };\
        key <RGHT>               {	[           Right ] };\
        key <END>                {	[             End ] };\
        key <DOWN>               {	[            Down ] };\
        key <PGDN>               {	[            Next ] };\
        key <INS>                {	[          Insert ] };\
        key <DELE>               {	[          Delete ] };\
        key <MUTE>               {	[   XF86AudioMute ] };\
        key <VOL->               {	[ XF86AudioLowerVolume ] };\
        key <VOL+>               {	[ XF86AudioRaiseVolume ] };\
        key <POWR>               {	[    XF86PowerOff ] };\
        key <KPEQ>               {	[        KP_Equal ] };\
        key <I126>               {	[       plusminus ] };\
        key <PAUS>               {\
                type= \"PC_CONTROL_LEVEL2\",\
                symbols[Group1]= [           Pause,           Break ]\
        };\
        key <I128>               {	[     XF86LaunchA ] };\
        key <I129>               {	[      KP_Decimal,      KP_Decimal ] };\
        key <HNGL>               {	[          Hangul ] };\
        key <HJCV>               {	[    Hangul_Hanja ] };\
        key <LWIN>               {	[         Super_L ] };\
        key <RWIN>               {	[         Super_R ] };\
        key <COMP>               {	[            Menu ] };\
        key <STOP>               {	[          Cancel ] };\
        key <AGAI>               {	[            Redo ] };\
        key <PROP>               {	[        SunProps ] };\
        key <UNDO>               {	[            Undo ] };\
        key <FRNT>               {	[        SunFront ] };\
        key <COPY>               {	[        XF86Copy ] };\
        key <OPEN>               {	[        XF86Open ] };\
        key <PAST>               {	[       XF86Paste ] };\
        key <FIND>               {	[            Find ] };\
        key <CUT>                {	[         XF86Cut ] };\
        key <HELP>               {	[            Help ] };\
        key <I147>               {	[      XF86MenuKB ] };\
        key <I148>               {	[  XF86Calculator ] };\
        key <I150>               {	[       XF86Sleep ] };\
        key <I151>               {	[      XF86WakeUp ] };\
        key <I152>               {	[    XF86Explorer ] };\
        key <I153>               {	[        XF86Send ] };\
        key <I155>               {	[        XF86Xfer ] };\
        key <I156>               {	[     XF86Launch1 ] };\
        key <I157>               {	[     XF86Launch2 ] };\
        key <I158>               {	[         XF86WWW ] };\
        key <I159>               {	[         XF86DOS ] };\
        key <I160>               {	[ XF86ScreenSaver ] };\
        key <I161>               {	[ XF86RotateWindows ] };\
        key <I162>               {	[    XF86TaskPane ] };\
        key <I163>               {	[        XF86Mail ] };\
        key <I164>               {	[   XF86Favorites ] };\
        key <I165>               {	[  XF86MyComputer ] };\
        key <I166>               {	[        XF86Back ] };\
        key <I167>               {	[     XF86Forward ] };\
        key <I169>               {	[       XF86Eject ] };\
        key <I170>               {	[       XF86Eject,       XF86Eject ] };\
        key <I171>               {	[   XF86AudioNext ] };\
        key <I172>               {	[   XF86AudioPlay,  XF86AudioPause ] };\
        key <I173>               {	[   XF86AudioPrev ] };\
        key <I174>               {	[   XF86AudioStop,       XF86Eject ] };\
        key <I175>               {	[ XF86AudioRecord ] };\
        key <I176>               {	[ XF86AudioRewind ] };\
        key <I177>               {	[       XF86Phone ] };\
        key <I179>               {	[       XF86Tools ] };\
        key <I180>               {	[    XF86HomePage ] };\
        key <I181>               {	[      XF86Reload ] };\
        key <I182>               {	[       XF86Close ] };\
        key <I185>               {	[    XF86ScrollUp ] };\
        key <I186>               {	[  XF86ScrollDown ] };\
        key <I187>               {	[       parenleft ] };\
        key <I188>               {	[      parenright ] };\
        key <I189>               {	[         XF86New ] };\
        key <I190>               {	[            Redo ] };\
        key <FK13>               {	[       XF86Tools ] };\
        key <FK14>               {	[     XF86Launch5 ] };\
        key <FK15>               {	[     XF86Launch6 ] };\
        key <FK16>               {	[     XF86Launch7 ] };\
        key <FK17>               {	[     XF86Launch8 ] };\
        key <FK18>               {	[     XF86Launch9 ] };\
        key <FK20>               {	[ XF86AudioMicMute ] };\
        key <FK21>               {	[ XF86TouchpadToggle ] };\
        key <FK22>               {	[  XF86TouchpadOn ] };\
        key <FK23>               {	[ XF86TouchpadOff ] };\
        key <MDSW>               {	[     Mode_switch ] };\
        key <ALT>                {	[        NoSymbol,           Alt_L ] };\
        key <META>               {	[        NoSymbol,          Meta_L ] };\
        key <SUPR>               {	[        NoSymbol,         Super_L ] };\
        key <HYPR>               {	[        NoSymbol,         Hyper_L ] };\
        key <I208>               {	[   XF86AudioPlay ] };\
        key <I209>               {	[  XF86AudioPause ] };\
        key <I210>               {	[     XF86Launch3 ] };\
        key <I211>               {	[     XF86Launch4 ] };\
        key <I212>               {	[     XF86LaunchB ] };\
        key <I213>               {	[     XF86Suspend ] };\
        key <I214>               {	[       XF86Close ] };\
        key <I215>               {	[   XF86AudioPlay ] };\
        key <I216>               {	[ XF86AudioForward ] };\
        key <I218>               {	[           Print ] };\
        key <I220>               {	[      XF86WebCam ] };\
        key <I221>               {	[ XF86AudioPreset ] };\
        key <I223>               {	[        XF86Mail ] };\
        key <I224>               {	[   XF86Messenger ] };\
        key <I225>               {	[      XF86Search ] };\
        key <I226>               {	[          XF86Go ] };\
        key <I227>               {	[     XF86Finance ] };\
        key <I228>               {	[        XF86Game ] };\
        key <I229>               {	[        XF86Shop ] };\
        key <I231>               {	[          Cancel ] };\
        key <I232>               {	[ XF86MonBrightnessDown ] };\
        key <I233>               {	[ XF86MonBrightnessUp ] };\
        key <I234>               {	[  XF86AudioMedia ] };\
        key <I235>               {	[     XF86Display ] };\
        key <I236>               {	[ XF86KbdLightOnOff ] };\
        key <I237>               {	[ XF86KbdBrightnessDown ] };\
        key <I238>               {	[ XF86KbdBrightnessUp ] };\
        key <I239>               {	[        XF86Send ] };\
        key <I240>               {	[       XF86Reply ] };\
        key <I241>               {	[ XF86MailForward ] };\
        key <I242>               {	[        XF86Save ] };\
        key <I243>               {	[   XF86Documents ] };\
        key <I244>               {	[     XF86Battery ] };\
        key <I245>               {	[   XF86Bluetooth ] };\
        key <I246>               {	[        XF86WLAN ] };\
        key <I247>               {	[         XF86UWB ] };\
        key <I254>               {	[        XF86WWAN ] };\
        key <I255>               {	[      XF86RFKill ] };\
        modifier_map Shift { <LFSH>, <RTSH> };\
        modifier_map Lock { <CAPS> };\
        modifier_map Control { <LCTL>, <RCTL> };\
        modifier_map Mod1 { <LALT>, <RALT>, <META> };\
        modifier_map Mod2 { <NMLK> };\
        modifier_map Mod4 { <LWIN>, <RWIN>, <SUPR>, <HYPR> };\
        modifier_map Mod5 { <LVL3>, <MDSW> };\
};\
\
};\
";
