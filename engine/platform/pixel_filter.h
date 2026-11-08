/* GPU pixel-art magnification. Scale3x rules: https://www.scale2x.it/algorithm
 * No external shader assets; GL 3.3 and GLES 3.0 use the same source. */
#ifndef EIDOS_PIXEL_FILTER_H
#define EIDOS_PIXEL_FILTER_H

#define PIXEL_FILTER_GLSL \
    "int pf_wrapc(int v, int size, int rep, int flip) {\n" \
    "    if (size <= 0) return 0;\n" \
    "    if (rep == 0) return clamp(v, 0, size - 1);\n" \
    "    if (flip != 0) {\n" \
    "        int k = v & (size * 2 - 1);\n" \
    "        if (k >= size) k = size * 2 - 1 - k;\n" \
    "        return k;\n" \
    "    }\n" \
    "    return v & (size - 1);\n" \
    "}\n" \
    "vec4 pf_fetch_wrap(sampler2D s, ivec2 p, ivec2 sz, ivec4 wrap) {\n" \
    "    ivec2 q = ivec2(pf_wrapc(p.x, sz.x, wrap.x, wrap.z), pf_wrapc(p.y, sz.y, wrap.y, wrap.w));\n" \
    "    vec4 c = texelFetch(s, q, 0);\n" \
    "    return c.a == 0.0 ? vec4(0.0) : c;\n" \
    "}\n" \
    "vec4 pf_fetch(sampler2D s, ivec2 p, ivec2 sz) {\n" \
    "    return pf_fetch_wrap(s, p, sz, ivec4(0));\n" \
    "}\n" \
    "vec4 pf_premul(vec4 c) { return vec4(c.rgb * c.a, c.a); }\n" \
    "vec4 pixel_sample_wrap(sampler2D s, vec2 p, ivec2 sz, ivec4 wrap, int mode) {\n" \
    "    ivec2 q = ivec2(floor(p));\n" \
    "    vec4 E = pf_fetch_wrap(s, q, sz, wrap);\n" \
    "    if (mode == 0) return E;\n" \
    "    if (mode == 1) {\n" \
    "        vec2 t = p - 0.5, f = fract(t); ivec2 b = ivec2(floor(t));\n" \
    "        f = f * f * (3.0 - 2.0 * f);\n" \
    "        vec4 c = mix(mix(pf_premul(pf_fetch_wrap(s,b,sz,wrap)), pf_premul(pf_fetch_wrap(s,b+ivec2(1,0),sz,wrap)),f.x),\n" \
    "                     mix(pf_premul(pf_fetch_wrap(s,b+ivec2(0,1),sz,wrap)), pf_premul(pf_fetch_wrap(s,b+ivec2(1,1),sz,wrap)),f.x),f.y);\n" \
    "        return c.a > 0.0 ? vec4(c.rgb / c.a, c.a) : vec4(0.0);\n" \
    "    }\n" \
    "    vec2 footprint = abs(dFdx(p)) + abs(dFdy(p));\n" \
    "    if (max(footprint.x, footprint.y) >= 1.0) return E;\n" \
    "    vec4 A=pf_fetch_wrap(s,q+ivec2(-1,-1),sz,wrap), B=pf_fetch_wrap(s,q+ivec2(0,-1),sz,wrap), C=pf_fetch_wrap(s,q+ivec2(1,-1),sz,wrap);\n" \
    "    vec4 D=pf_fetch_wrap(s,q+ivec2(-1,0),sz,wrap), F=pf_fetch_wrap(s,q+ivec2(1,0),sz,wrap);\n" \
    "    vec4 G=pf_fetch_wrap(s,q+ivec2(-1,1),sz,wrap), H=pf_fetch_wrap(s,q+ivec2(0,1),sz,wrap), I=pf_fetch_wrap(s,q+ivec2(1,1),sz,wrap);\n" \
    "    if (B == H || D == F) return E;\n" \
    "    ivec2 sub = ivec2(floor(fract(p) * 3.0));\n" \
    "    int k = sub.y * 3 + sub.x;\n" \
    "    if (k==0 && D==B) return D;\n" \
    "    if (k==1 && ((D==B && E!=C) || (B==F && E!=A))) return B;\n" \
    "    if (k==2 && B==F) return F;\n" \
    "    if (k==3 && ((D==B && E!=G) || (D==H && E!=A))) return D;\n" \
    "    if (k==5 && ((B==F && E!=I) || (H==F && E!=C))) return F;\n" \
    "    if (k==6 && D==H) return D;\n" \
    "    if (k==7 && ((D==H && E!=I) || (H==F && E!=G))) return H;\n" \
    "    if (k==8 && H==F) return F;\n" \
    "    return E;\n" \
    "}\n" \
    "vec4 pixel_sample(sampler2D s, vec2 p, ivec2 sz, int mode) {\n" \
    "    return pixel_sample_wrap(s, p, sz, ivec4(0), mode);\n" \
    "}\n"

#endif
