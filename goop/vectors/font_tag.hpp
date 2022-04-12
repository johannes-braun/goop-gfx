#pragma once

#include <cstdint>

namespace goop
{
  consteval std::uint32_t make_u32tag(char const (&arr)[5])
  {
    return std::uint32_t(arr[3] | (arr[2] << 8) | (arr[1] << 16) | (arr[0] << 24));
  }

  enum class font_tag : std::uint32_t 
  {
    table_avar = make_u32tag("avar"),
    table_base = make_u32tag("BASE"),
    table_cbdt = make_u32tag("CBDT"),
    table_cblc = make_u32tag("CBLC"),
    table_cff  = make_u32tag("CFF "),
    table_cff2 = make_u32tag("CFF2"),
    table_cmap = make_u32tag("cmap"),
    table_colr = make_u32tag("COLR"),
    table_cpal = make_u32tag("CPAL"),
    table_cvar = make_u32tag("cvar"),
    table_cvt  = make_u32tag("cvt "),
    table_dsig = make_u32tag("DSIG"),
    table_ebdt = make_u32tag("EBDT"),
    table_eblc = make_u32tag("EBLC"),
    table_ebsc = make_u32tag("EBSC"),
    table_fpgm = make_u32tag("fpgm"),
    table_fvar = make_u32tag("fvar"),
    table_gasp = make_u32tag("gasp"),
    table_gdef = make_u32tag("GDEF"),
    table_glyf = make_u32tag("glyf"),
    table_gpos = make_u32tag("GPOS"),
    table_gsub = make_u32tag("GSUB"),
    table_gvar = make_u32tag("gvar"),
    table_hdmx = make_u32tag("hdmx"),
    table_head = make_u32tag("head"),
    table_hhea = make_u32tag("hhea"),
    table_hmtx = make_u32tag("hmtx"),
    table_hvar = make_u32tag("HVAR"),
    table_jstf = make_u32tag("JSTF"),
    table_kern = make_u32tag("kern"),
    table_loca = make_u32tag("loca"),
    table_ltsh = make_u32tag("LTSH"),
    table_math = make_u32tag("MATH"),
    table_name = make_u32tag("name"),
    table_os_2 = make_u32tag("OS_2"),
    table_maxp = make_u32tag("maxp"),
    table_merg = make_u32tag("MERG"),
    table_meta = make_u32tag("meta"),
    table_mvar = make_u32tag("MVAR"),
    table_pclt = make_u32tag("PCLT"),
    table_post = make_u32tag("post"),
    table_prep = make_u32tag("prep"),
    table_sbix = make_u32tag("sbix"),
    table_stat = make_u32tag("STAT"),
    table_svg  = make_u32tag("SVG "),
    table_vdmx = make_u32tag("VDMX"),
    table_vhea = make_u32tag("vhea"),
    table_vmtx = make_u32tag("vmtx"),
    table_vorg = make_u32tag("VORG"),
    table_vvar = make_u32tag("VVAR"),
  };

  consteval font_tag make_tag(char const (&arr)[5])
  {
    return font_tag{ make_u32tag(arr) };
  }
}