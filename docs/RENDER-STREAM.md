# How the glass travels from the application to the render server (macOS 26.7.2, read with REA)

The application's QuartzCore encodes its layer tree and sends it to the render server (WindowServer) as the commit stream (messages 0x9c42..0x9c45), which `WindowServer/src/ca_scene.c` decodes. Object type ids come from `CA::Render::Decoder::decode_object` (0x7ff80d34f394 in the extracted QuartzCore): `0x12` Filter, `0x1d` KeyValue, `0x1e` Layer, `0x2a` PortalLayer, `0x2f` SDFElementLayer, `0x30` SDFLayer, `0x0c` DistanceFieldLayer, `0x33` ShapeLayer, `0x36` String, `0x3e` Vector, `0x14` GradientLayer, `0x03` BackdropLayer.

## Filter (type 0x12, `CA::Render::Filter::decode`, 0x7ff80d1b6988)
`i32 flags`; if `flags bit 8` is set: `atom` = a shared (cached) filter by that atom. Else `i8 kind`:
- `1`: `atom` (the **filter type**, the atom of "glassBackground"...), an **object of type 0x36** (the name String), then an **array of KeyValue objects** (type 0x1d, `decode_array_object`): the keys and their values. The Filter is built from (type atom, name, KeyValueArray); `flags << 8` is OR-ed into its own flags. The atoms of the keys are the `CAInternAtom` ids of QuartzCore/src/CAFilterGlass.inc (0x182 = inputInnerRefractionAmount ...).
- `0`: a CF data object holding an archived CIFilter (ignored when the decoder is in a restricted mode).
ca_scene.c already walks this record (case 0x12) but throws it away; the work is to keep (type atom, [(key atom, value)]) in the layer.
KeyValue values: floats / ints / colours / vec2 are objects; `KeyValueArray::get_float_key(array, atom, default)` is what the renderer calls (the defaults are the ones in docs/APPLE-DECISIONS.md).

## SDFLayer (type 0x30, `SDFLayer::SDFLayer(Decoder*)`, 0x7ff80d33016e)
`Object` header (flags), then **0x70 bytes of effect parameters** (the record that `-[CASDF*Effect configureLayer:transaction:]` fills: for glass displacement the floats height, curvature, angle, maskOffset; the effect kind byte, 7 = glass displacement, is the enum below), `i8 effect kind` (0..8; kind 3 is followed by a Gradient object), `i32` (smoothness bits), `i32` (gaussianRadius bits), `i32` (effectOffset bits), `i8` (mergeElements), `vm_size_t` (id of the element list).

## SDFElementLayer (type 0x2f, 0x7ff80d33032a)
`Object` header, `i8 mode` (0..1), `i8 operation` (0..1), `i32` contentsZeroValueDistance bits, `i32` contentsOneValueDistance bits (default 1.0f), `i32` gradientOvalization bits, `i8` hitTestsAsFill.

## What is verified
The formats are read from the decompilation. **Not verified on a real stream**: no capture with a glass filter exists yet (the lab Mac has no GPU, so AppKit does not create the glass there). To get one: run a small Mac program that puts a `glassBackground` CAFilter on a layer and commits it, through darwinator with the commit-capturing stub, and decode the stream with ca_scene.c.
