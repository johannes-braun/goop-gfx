#pragma once

#include <cstdint>

namespace goop
{
  consteval std::uint32_t make_u32tag(char const (&arr)[5])
  {
    return std::uint32_t(arr[3] | (arr[2] << 8) | (arr[1] << 16) | (arr[0] << 24));
  }

  enum class font_table : std::uint32_t 
  {
    tab_avar = make_u32tag("avar"),
    tab_base = make_u32tag("BASE"),
    tab_cbdt = make_u32tag("CBDT"),
    tab_cblc = make_u32tag("CBLC"),
    tab_cff  = make_u32tag("CFF "),
    tab_cff2 = make_u32tag("CFF2"),
    tab_cmap = make_u32tag("cmap"),
    tab_colr = make_u32tag("COLR"),
    tab_cpal = make_u32tag("CPAL"),
    tab_cvar = make_u32tag("cvar"),
    tab_cvt  = make_u32tag("cvt "),
    tab_dsig = make_u32tag("DSIG"),
    tab_ebdt = make_u32tag("EBDT"),
    tab_eblc = make_u32tag("EBLC"),
    tab_ebsc = make_u32tag("EBSC"),
    tab_fpgm = make_u32tag("fpgm"),
    tab_fvar = make_u32tag("fvar"),
    tab_gasp = make_u32tag("gasp"),
    tab_gdef = make_u32tag("GDEF"),
    tab_glyf = make_u32tag("glyf"),
    tab_gpos = make_u32tag("GPOS"),
    tab_gsub = make_u32tag("GSUB"),
    tab_gvar = make_u32tag("gvar"),
    tab_hdmx = make_u32tag("hdmx"),
    tab_head = make_u32tag("head"),
    tab_hhea = make_u32tag("hhea"),
    tab_hmtx = make_u32tag("hmtx"),
    tab_hvar = make_u32tag("HVAR"),
    tab_jstf = make_u32tag("JSTF"),
    tab_kern = make_u32tag("kern"),
    tab_loca = make_u32tag("loca"),
    tab_ltsh = make_u32tag("LTSH"),
    tab_math = make_u32tag("MATH"),
    tab_name = make_u32tag("name"),
    tab_os_2 = make_u32tag("OS_2"),
    tab_maxp = make_u32tag("maxp"),
    tab_merg = make_u32tag("MERG"),
    tab_meta = make_u32tag("meta"),
    tab_mvar = make_u32tag("MVAR"),
    tab_pclt = make_u32tag("PCLT"),
    tab_post = make_u32tag("post"),
    tab_prep = make_u32tag("prep"),
    tab_sbix = make_u32tag("sbix"),
    tab_stat = make_u32tag("STAT"),
    tab_svg  = make_u32tag("SVG "),
    tab_vdmx = make_u32tag("VDMX"),
    tab_vhea = make_u32tag("vhea"),
    tab_vmtx = make_u32tag("vmtx"),
    tab_vorg = make_u32tag("VORG"),
    tab_vvar = make_u32tag("VVAR"),
  };
}