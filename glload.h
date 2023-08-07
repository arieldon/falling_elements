#ifndef GL_LOAD_H
#define GL_LOAD_H

/* ---
 * NOTE(ariel) This OpenGL extension loader is based on Fabian Giesen's Bink GL
 * loader.
 * https://gist.github.com/rygorous/16796a0c876cf8a5f542caddb55bce8a
 * ---
 */

#include <dlfcn.h>

typedef void (APIENTRY *DEBUGPROC)(GLenum source, GLenum Type, GLuint Id, GLenum Severity,
	GLsizei Length, const GLchar *Message, const void *UserParam);

#define DESIRED_GL_PROCEDURES \
	GLE(GLuint, CreateProgram, void) \
	GLE(GLuint, CreateShader, GLenum type) \
	GLE(void, AttachShader, GLuint program, GLuint shader) \
	GLE(void, BindBuffer, GLenum target, GLuint buffer) \
	GLE(void, BindVertexArray, GLuint array) \
	GLE(void, BufferData, GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage) \
	GLE(void, CompileShader, GLuint shader) \
	GLE(void, DebugMessageCallback, DEBUGPROC callback, const void *UserParam) \
	GLE(void, DebugMessageControl, GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled) \
	GLE(void, EnableVertexAttribArray, GLuint index) \
	GLE(void, GenBuffers, GLsizei n, GLuint *buffers) \
	GLE(void, GenVertexArrays, GLsizei n, GLuint *arrays) \
	GLE(void, GetProgramiv, GLuint program, GLenum pname, GLint *params) \
	GLE(void, GetShaderiv, GLuint shader, GLenum pname, GLint *params) \
	GLE(void, LinkProgram, GLuint program) \
	GLE(void, ShaderSource, GLuint shader, GLsizei count, const GLchar* const *string, const GLint *length) \
	GLE(void, UseProgram, GLuint program) \
	GLE(void, VertexAttribPointer, GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid * pointer) \

// NOTE(ariel) Concatenate procedure name (`Name`) with "proc" to get the name
// of the procedure in the dynamic library.
#define GLE(ReturnType, Name, ...) typedef ReturnType Name##proc(__VA_ARGS__); extern Name##proc *gl##Name;
DESIRED_GL_PROCEDURES
#undef GLE

#define GLE(ReturnType, Name, ...) Name##proc *gl##Name;
DESIRED_GL_PROCEDURES
#undef GLE

static void
LoadOpenGLExtensions(void)
{
	void *GLSharedObject = dlopen("libGL.so", RTLD_LAZY);
	Assert(GLSharedObject);

#define GLE(ReturnType, Name, ...) \
	gl##Name = (Name##proc *) dlsym(GLSharedObject, "gl" #Name); \
	Assert(gl##Name);
	DESIRED_GL_PROCEDURES
#undef GLE
}

#endif
