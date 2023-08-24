static s32 QuadsCount;
static quad Quads[ArrayCount(CellBuffer) + ArrayCount(MenuContext.Commands)];

#ifdef DEBUG
static void
OpenGLDebugMessageCallback(
	GLenum Source,
	GLenum Type,
	GLuint ID,
	GLenum Severity,
	GLsizei Length,
	const GLchar *Message,
	const void *UserParam)
{
	(void)Source;
	(void)ID;
	(void)Severity;
	(void)Length;
	(void)UserParam;
	if (Type == GL_DEBUG_TYPE_ERROR)
	{
		fprintf(stderr, "GL ERROR: %s\n", Message);
	}
	else
	{
		fprintf(stderr, "GL INFO: %s\n", Message);
	}
}
#endif

static void
InitializeRenderer(renderer_context *Context)
{
	LoadOpenGLExtensions();

	// NOTE(ariel) Set global state of OpenGL context.
	glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#ifdef DEBUG
	{
		s32 OpenGLMajorVersion = 0;
		s32 OpenGLMinorVersion = 0;
		glGetIntegerv(GL_MAJOR_VERSION, &OpenGLMajorVersion);
		glGetIntegerv(GL_MINOR_VERSION, &OpenGLMinorVersion);
		fprintf(stderr, "OpenGL %d.%d: %s %s\n",
			OpenGLMajorVersion, OpenGLMinorVersion,
			glGetString(GL_VENDOR), glGetString(GL_RENDERER));
	}

	// NOTE(ariel) Output any and all debug messages from OpenGL.
	{
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(OpenGLDebugMessageCallback, 0);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, 0, GL_TRUE);
	}
#endif

	// NOTE(ariel) Build and link shader program.
	enum
	{
		BASE_POSITION = 0,
		BASE_TEXTURE_COORDINATES = 1,
		INSTANCE_QUAD = 2,
		INSTANCE_COLOR = 3,
		INSTANCE_TEXTURE_ID = 4,
	};
	{
		const char *VertexShaderSource =
			"#version 330 core\n"
			"uniform vec2 WindowDimensions;\n"
			// NOTE(ariel) These locations _must_ match definition of enum above.
			"layout (location = 0) in vec2 BasePosition;\n"
			"layout (location = 1) in vec2 BaseTextureCoordinates;\n"
			"layout (location = 2) in vec4 InstanceQuad;\n"
			"layout (location = 3) in vec4 InstanceColor;\n"
			"layout (location = 4) in float InstanceTextureID;\n"
			"out vec2 UVCoordinatesForFragmentShader;\n"
			"out vec4 ColorForFragmentShader;\n"
			"void main()\n"
			"{\n"
			"	UVCoordinatesForFragmentShader = vec2(0.25f*InstanceTextureID, 0.0f) + 0.25f*BaseTextureCoordinates;\n"
			"	ColorForFragmentShader = InstanceColor;\n"
			// NOTE(ariel) Scale and translate base.
			"	vec2 Offset = InstanceQuad.xy;\n"
			"	vec2 Scale = InstanceQuad.zw;\n"
			"	vec2 Translation = Offset + Scale/2.0f;\n"
			"	vec2 CellPosition = Scale*BasePosition + Translation;\n"
			// NOTE(ariel) Split standard orthographic projection matrix into two
			// transforms: first scale, second translate.
			"	CellPosition.x *= 2.0f / WindowDimensions.x;\n"
			"	CellPosition.x += -1.0f;\n"
			"	CellPosition.y *= -2.0f / WindowDimensions.y;\n"
			"	CellPosition.y += 1.0f;\n"
			"	gl_Position = vec4(CellPosition, 0.0f, 1.0f);\n"
			"}\n";
		GLuint VertexShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(VertexShader, 1, &VertexShaderSource, 0);
		glCompileShader(VertexShader);

		const char *FragmentShaderSource =
			"#version 330 core\n"
			"uniform sampler2D Icons;\n"
			"in vec2 UVCoordinatesForFragmentShader;\n"
			"in vec4 ColorForFragmentShader;\n"
			"out vec4 OutputColor;\n"
			"void main()\n"
			"{\n"
			"	OutputColor = texture(Icons, UVCoordinatesForFragmentShader).r * ColorForFragmentShader;\n"
			"}\n";
		GLuint FragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(FragmentShader, 1, &FragmentShaderSource, 0);
		glCompileShader(FragmentShader);

		GLint CompileStatus = 0;
		glGetShaderiv(VertexShader, GL_COMPILE_STATUS, &CompileStatus); Assert(CompileStatus);
		glGetShaderiv(FragmentShader, GL_COMPILE_STATUS, &CompileStatus); Assert(CompileStatus);

		GLuint ShaderProgram = glCreateProgram();
		glAttachShader(ShaderProgram, VertexShader);
		glAttachShader(ShaderProgram, FragmentShader);
		glLinkProgram(ShaderProgram);

		GLint LinkStatus = 0;
		glGetProgramiv(ShaderProgram, GL_LINK_STATUS, &LinkStatus); Assert(LinkStatus);

		glUseProgram(ShaderProgram);

		GLint UniformWindowLocation = glGetUniformLocation(ShaderProgram, "WindowDimensions"); Assert(UniformWindowLocation != -1);
		glUniform2f(UniformWindowLocation, WINDOW_WIDTH, WINDOW_HEIGHT);

		glDeleteShader(FragmentShader);
		glDeleteShader(VertexShader);

		Context->ShaderProgram = ShaderProgram;
	}

	// NOTE(ariel) Allocate and load arrays on GPU.
	{
		GLuint VertexArray = 0;
		glGenVertexArrays(1, &VertexArray);
		glBindVertexArray(VertexArray);

		GLuint Texture = 0;
		glActiveTexture(GL_TEXTURE0);
		glGenTextures(1, &Texture);
		glBindTexture(GL_TEXTURE_2D, Texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 128, 128, 0, GL_RED, GL_UNSIGNED_BYTE, IconBitmap);

		f32 BaseQuadVertices[] =
		{
			+0.5f, +0.5f, +1.0f, +1.0f,
			+0.5f, -0.5f, +1.0f, +0.0f,
			-0.5f, +0.5f, +0.0f, +1.0f,

			+0.5f, -0.5f, +1.0f, +0.0f,
			-0.5f, -0.5f, +0.0f, +0.0f,
			-0.5f, +0.5f, +0.0f, +1.0f,
		};
		GLuint VerticesBuffer = 0;
		glGenBuffers(1, &VerticesBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, VerticesBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(BaseQuadVertices), BaseQuadVertices, GL_STATIC_DRAW);

		glEnableVertexAttribArray(BASE_POSITION);
		glVertexAttribPointer(BASE_POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(f32) * 4, 0);
		glVertexAttribDivisor(BASE_POSITION, 0);

		glEnableVertexAttribArray(BASE_TEXTURE_COORDINATES);
		glVertexAttribPointer(BASE_TEXTURE_COORDINATES, 2, GL_FLOAT, GL_FALSE, sizeof(f32) * 4, (void *)(sizeof(f32) * 2));
		glVertexAttribDivisor(BASE_TEXTURE_COORDINATES, 0);

		GLuint InstancesBuffer = 0;
		glGenBuffers(1, &InstancesBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, InstancesBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Quads), 0, GL_STREAM_DRAW);

		glEnableVertexAttribArray(INSTANCE_QUAD);
		glVertexAttribPointer(INSTANCE_QUAD, 4, GL_INT, GL_FALSE,
			sizeof(quad), (void *)0);
		glVertexAttribDivisor(INSTANCE_QUAD, 1);

		glEnableVertexAttribArray(INSTANCE_COLOR);
		glVertexAttribPointer(INSTANCE_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE,
			sizeof(quad), (void *)offsetof(quad, Color));
		glVertexAttribDivisor(INSTANCE_COLOR, 1);

		glEnableVertexAttribArray(INSTANCE_TEXTURE_ID);
		glVertexAttribPointer(INSTANCE_TEXTURE_ID, 1, GL_UNSIGNED_BYTE, GL_FALSE,
			sizeof(quad), (void *)offsetof(quad, TextureID));
		glVertexAttribDivisor(INSTANCE_TEXTURE_ID, 1);

		Context->Texture = Texture;
		Context->VertexArray = VertexArray;
		Context->InstancesBuffer = InstancesBuffer;
		Context->BaseVerticesBuffer = VerticesBuffer;
	}

	Assert(glGetError() == GL_NO_ERROR);
}

static void
TerminateRenderer(renderer_context Context)
{
	glDeleteProgram(Context.ShaderProgram);
	glDeleteTextures(1, &Context.Texture);
	glDeleteBuffers(1, &Context.InstancesBuffer);
	glDeleteBuffers(1, &Context.BaseVerticesBuffer);
	glDeleteVertexArrays(1, &Context.VertexArray);
}

static void
PresentBuffer(void)
{
	glClear(GL_COLOR_BUFFER_BIT);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Quads[0]) * QuadsCount, Quads);
	glDrawArraysInstanced(GL_TRIANGLES, 0, 6, QuadsCount);
	glXSwapBuffers(X11Display, X11Window); // TODO(ariel) Use vertical sync if it's available?
}

static inline u64
GetTime(void)
{
	u64 Nanoseconds = 0;
	struct timespec Now = {0};
	clock_gettime(CLOCK_MONOTONIC, &Now);
	Nanoseconds += Now.tv_sec;
	Nanoseconds *= NANOSECONDS_PER_SECOND;
	Nanoseconds += Now.tv_nsec;
	return Nanoseconds;
}

static inline void
SleepForNanoseconds(u64 DeltaTimeNS)
{
	static const u64 TARGET_FRAME_TIME_NS = NANOSECONDS_PER_SECOND / TARGET_FRAMES_PER_SECOND;
	s64 SleepTimeNS = TARGET_FRAME_TIME_NS - DeltaTimeNS;
	if (SleepTimeNS > 0)
	{
		struct timespec Time = {0};
		Time.tv_sec = SleepTimeNS / NANOSECONDS_PER_SECOND;
		Time.tv_nsec = SleepTimeNS % NANOSECONDS_PER_SECOND;
		nanosleep(&Time, 0);
	}
}
