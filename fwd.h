#pragma once

// fwd.h - forward declarations

typedef void* (*GLADloadproc)(const char* name);

namespace window {
	struct Event;
}

namespace audio {
	struct Event;
}

namespace graphics {
	struct VertexShader;
	struct FragmentShader;
	struct VertexInput;
	struct Buffer;
	struct Texture;
	struct Sampler;
	struct Framebuffer;
	struct Viewport;
}
