#ifdef DEBUG
static void
OpenGLDebugMessageCallback(
	GLenum Source,
	GLenum Type,
	GLuint Id,
	GLenum Severity,
	GLsizei Length,
	const GLchar *Message,
	const void *UserParam)
{
	(void)Source;
	(void)Id;
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

// FIXME(ariel) Free any resources allocated. I don't believe GPU memory works
// like CPU memory, namely shader program and stuff like that.
static void
InitializeRenderer(void)
{
	LoadOpenGLExtensions();

	// NOTE(ariel) Set global state of OpenGL context.
	glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	glDisable(GL_BLEND);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

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
	enum { BASE_POSITION = 0, INSTANCE_OFFSET = 1, INSTANCE_COLOR = 2 };
	{
		const char *VertexShaderSource =
			"#version 330 core\n"
			"layout (location = 0) in vec2 BasePosition;\n"
			"layout (location = 1) in vec2 InstanceOffset;\n"
			"layout (location = 2) in vec4 InstanceColor;\n"
			"out vec4 ColorForFragmentShader;\n"
			"void main()\n"
			"{\n"
			"	ColorForFragmentShader = InstanceColor;\n"
			"	vec2 CellPosition = BasePosition + InstanceOffset;\n"
			// NOTE(ariel) Split standard orthographic projection matrix into two
			// matrices: first scale, second translate.
			"	CellPosition.x *= 2.0f / 1920.0f;\n"
			"	CellPosition.x += -1;\n"
			"	CellPosition.y *= -2.0f / 1080.0f;\n"
			"	CellPosition.y += 1;\n"
			"	gl_Position = vec4(CellPosition, 0.0f, 1.0f);\n"
			"}\n";
		GLuint VertexShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(VertexShader, 1, &VertexShaderSource, 0);
		glCompileShader(VertexShader);

		const char *FragmentShaderSource =
			"#version 330 core\n"
			"in vec4 ColorForFragmentShader;\n"
			"out vec4 OutputColor;\n"
			"void main()\n"
			"{\n"
			"	OutputColor = ColorForFragmentShader;\n"
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

		glDeleteShader(FragmentShader);
		glDeleteShader(VertexShader);
	}

	// NOTE(ariel) Allocate and load arrays on GPU.
	{
		GLuint VertexArray = 0;
		glGenVertexArrays(1, &VertexArray);
		glBindVertexArray(VertexArray);

		f32 BaseQuadVertices[] =
		{
			+4.0f, +4.0f,
			+4.0f, -4.0f,
			-4.0f, +4.0f,

			+4.0f, -4.0f,
			-4.0f, -4.0f,
			-4.0f, +4.0f,
		};
		GLuint VerticesBuffer = 0;
		glGenBuffers(1, &VerticesBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, VerticesBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(BaseQuadVertices), BaseQuadVertices, GL_STATIC_DRAW);

		glEnableVertexAttribArray(BASE_POSITION);
		glVertexAttribPointer(BASE_POSITION, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glVertexAttribDivisor(BASE_POSITION, 0);

		GLuint InstancesBuffer = 0;
		glGenBuffers(1, &InstancesBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, InstancesBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Quads), 0, GL_STREAM_DRAW);

		glEnableVertexAttribArray(INSTANCE_OFFSET);
		glVertexAttribPointer(INSTANCE_OFFSET, 2, GL_INT, GL_FALSE, sizeof(Quads[0]), (void *)0);
		glVertexAttribDivisor(INSTANCE_OFFSET, 1);

		glEnableVertexAttribArray(INSTANCE_COLOR);
		glVertexAttribPointer(INSTANCE_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Quads[0]), (void *)(sizeof(s32) * 2));
		glVertexAttribDivisor(INSTANCE_COLOR, 1);
	}

	Assert(glGetError() == GL_NO_ERROR);
}

static void
PresentBuffer(void)
{
	glClear(GL_COLOR_BUFFER_BIT);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Quads[0]) * QuadsCount, Quads);
	glDrawArraysInstanced(GL_TRIANGLES, 0, 6, QuadsCount);
	glXSwapBuffers(X11Display, X11Window);
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
