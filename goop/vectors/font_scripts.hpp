#pragma once

#include "font_tag.hpp"

namespace goop
{
  enum class font_script : std::uint32_t
  {
    adlam = make_u32tag("adlm"),
    ahom = make_u32tag("ahom"),
    anatolian_hieroglyphs = make_u32tag("hluw"),
    arabic = make_u32tag("arab"),
    armenian = make_u32tag("armn"),
    avestan = make_u32tag("avst"),
    balinese = make_u32tag("bali"),
    bamum = make_u32tag("bamu"),
    bassa_vah = make_u32tag("bass"),
    batak = make_u32tag("batk"),
    bengali = make_u32tag("beng"),
    bengali_v2 = make_u32tag("bng2"),
    bhaiksuki = make_u32tag("bhks"),
    bopomofo = make_u32tag("bopo"),
    brahmi = make_u32tag("brah"),
    braille = make_u32tag("brai"),
    buginese = make_u32tag("bugi"),
    buhid = make_u32tag("buhd"),
    byzantine_music = make_u32tag("byzm"),
    canadian_syllabics = make_u32tag("cans"),
    carian = make_u32tag("cari"),
    caucasian_albanian = make_u32tag("aghb"),
    chakma = make_u32tag("cakm"),
    cham = make_u32tag("cham"),
    cherokee = make_u32tag("cher"),
    chorasmian = make_u32tag("chrs"),
    cjk_ideographic = make_u32tag("hani"),
    coptic = make_u32tag("copt"),
    cypriot_syllabary = make_u32tag("cprt"),
    cypro_minoan = make_u32tag("cpmn"),
    cyrillic = make_u32tag("cyrl"),
    default_script = make_u32tag("DFLT"),
    deseret = make_u32tag("dsrt"),
    devanagari = make_u32tag("deva"),
    devanagari_v2 = make_u32tag("dev2"),
    dives_akuru = make_u32tag("diak"),
    dogra = make_u32tag("dogr"),
    duployan = make_u32tag("dupl"),
    egyptian_hieroglyphs = make_u32tag("egyp"),
    elbasan = make_u32tag("elba"),
    elymaic = make_u32tag("elym"),
    ethiopic = make_u32tag("ethi"),
    georgian = make_u32tag("geor"),
    glagolitic = make_u32tag("glag"),
    gothic = make_u32tag("goth"),
    grantha = make_u32tag("gran"),
    greek = make_u32tag("grek"),
    gujarati = make_u32tag("gujr"),
    gujarati_v2 = make_u32tag("gjr2"),
    gunjala_gondi = make_u32tag("gong"),
    gurmukhi = make_u32tag("guru"),
    gurmukhi_v2 = make_u32tag("gur2"),
    hangul = make_u32tag("hang"),
    hangul_jamo = make_u32tag("jamo"), // use of this tag is not recommended. fonts supporting_unicode conjoining jamo characters should use 'hang' instead.
    hanifi_rohingya = make_u32tag("rohg"),
    hanunoo = make_u32tag("hano"),
    hatran = make_u32tag("hatr"),
    hebrew = make_u32tag("hebr"),
    hiragana = make_u32tag("kana"),
    imperial_aramaic = make_u32tag("armi"),
    inscriptional_pahlavi = make_u32tag("phli"),
    inscriptional_parthian = make_u32tag("prti"),
    javanese = make_u32tag("java"),
    kaithi = make_u32tag("kthi"),
    kannada = make_u32tag("knda"),
    kannada_v2 = make_u32tag("knd2"),
    katakana = make_u32tag("kana"),
    kayah_li = make_u32tag("kali"),
    kharosthi = make_u32tag("khar"),
    khitan_small_script = make_u32tag("kits"),
    khmer = make_u32tag("khmr"),
    khojki = make_u32tag("khoj"),
    khudawadi = make_u32tag("sind"),
    lao = make_u32tag("lao "),
    latin = make_u32tag("latn"),
    lepcha = make_u32tag("lepc"),
    limbu = make_u32tag("limb"),
    linear_a = make_u32tag("lina"),
    linear_b = make_u32tag("linb"),
    lisu = make_u32tag("lisu"),
    lycian = make_u32tag("lyci"),
    lydian = make_u32tag("lydi"),
    mahajani = make_u32tag("mahj"),
    makasar = make_u32tag("maka"),
    malayalam = make_u32tag("mlym"),
    malayalam_v2 = make_u32tag("mlm2"),
    mandaic, _mandaean = make_u32tag("mand"),
    manichaean = make_u32tag("mani"),
    marchen = make_u32tag("marc"),
    masaram_gondi = make_u32tag("gonm"),
    mathematical_alphanumeric_symbols = make_u32tag("math"),
    medefaidrin = make_u32tag("medf"),
    meitei_mayek = make_u32tag("mtei"),
    mende_kikakui = make_u32tag("mend"),
    meroitic_cursive = make_u32tag("merc"),
    meroitic_hieroglyphs = make_u32tag("mero"),
    miao = make_u32tag("plrd"),
    modi = make_u32tag("modi"),
    mongolian = make_u32tag("mong"),
    mro = make_u32tag("mroo"),
    multani = make_u32tag("mult"),
    musical_symbols = make_u32tag("musc"),
    myanmar = make_u32tag("mymr"),                                 // use of this tag is not recommended. fonts supporting_unicode encoding of myanmar script should use 'myr2' instead.
    myanmar_v2 = make_u32tag("mym2"),
    nabataean = make_u32tag("nbat"),
    nandinagari = make_u32tag("nand"),
    newa = make_u32tag("newa"),
    new_tai_lue = make_u32tag("talu"),
    n_ko = make_u32tag("nko "),
    n�shu = make_u32tag("nshu"),
    nyiakeng_puachue_hmong = make_u32tag("hmnp"),
    odia = make_u32tag("orya"),
    odia_v2 = make_u32tag("ory2"),
    ogham = make_u32tag("ogam"),
    ol_chiki = make_u32tag("olck"),
    old_italic = make_u32tag("ital"),
    old_hungarian = make_u32tag("hung"),
    old_north_arabian = make_u32tag("narb"),
    old_permic = make_u32tag("perm"),
    old_persian_cuneiform = make_u32tag("xpeo"),
    old_sogdian = make_u32tag("sogo"),
    old_south_arabian = make_u32tag("sarb"),
    old_turkic, _orkhon_runic = make_u32tag("orkh"),
    old_uyghur = make_u32tag("ougr"),
    osage = make_u32tag("osge"),
    osmanya = make_u32tag("osma"),
    pahawh_hmong = make_u32tag("hmng"),
    palmyrene = make_u32tag("palm"),
    pau_cin_hau = make_u32tag("pauc"),
    phags_pa = make_u32tag("phag"),
    phoenician = make_u32tag("phnx"),
    psalter_pahlavi = make_u32tag("phlp"),
    rejang = make_u32tag("rjng"),
    runic = make_u32tag("runr"),
    samaritan = make_u32tag("samr"),
    saurashtra = make_u32tag("saur"),
    sharada = make_u32tag("shrd"),
    shavian = make_u32tag("shaw"),
    siddham = make_u32tag("sidd"),
    sign_writing = make_u32tag("sgnw"),
    sinhala = make_u32tag("sinh"),
    sogdian = make_u32tag("sogd"),
    sora_sompeng = make_u32tag("sora"),
    soyombo = make_u32tag("soyo"),
    sumero_akkadian_cuneiform = make_u32tag("xsux"),
    sundanese = make_u32tag("sund"),
    syloti_nagri = make_u32tag("sylo"),
    syriac = make_u32tag("syrc"),
    tagalog = make_u32tag("tglg"),
    tagbanwa = make_u32tag("tagb"),
    tai_le = make_u32tag("tale"),
    tai_tham = make_u32tag("lana"),
    tai_viet = make_u32tag("tavt"),
    takri = make_u32tag("takr"),
    tamil = make_u32tag("taml"),
    tamil_v2 = make_u32tag("tml2"),
    tangsa = make_u32tag("tnsa"),
    tangut = make_u32tag("tang"),
    telugu = make_u32tag("telu"),
    telugu_v2 = make_u32tag("tel2"),
    thaana = make_u32tag("thaa"),
    thai = make_u32tag("thai"),
    tibetan = make_u32tag("tibt"),
    tifinagh = make_u32tag("tfng"),
    tirhuta = make_u32tag("tirh"),
    toto = make_u32tag("toto"),
    ugaritic_cuneiform = make_u32tag("ugar"),
    vai = make_u32tag("vai "),
    vithkuqi = make_u32tag("vith"),
    wancho = make_u32tag("wcho"),
    warang_citi = make_u32tag("wara"),
    yezidi = make_u32tag("yezi"),
    yi = make_u32tag("yi\x20\x20"),
    zanabazar_square = make_u32tag("zanb"),
  };
}