#include <FIo.h>
#include <math.h>
#include "GlesShader.h"
#include "fragment.h"
#include "vertex.h"


using namespace Tizen::App;
using namespace Tizen::Base;
using namespace Tizen::Base::Runtime;
using namespace Tizen::Base::Utility;
using namespace Tizen::System;
using namespace Tizen::Ui;
using namespace Tizen::Ui::Controls;
using namespace Tizen::Graphics;
using namespace Tizen::Graphics::Opengl;


const int TIME_OUT = 10;
const int PAUSE_TIME = 100;
const double PI = 3.1415926535897932384626433832795;

extern const float VERTEX_ATTRIBUTES[];
extern const float VERTEX_1[];

extern const float TexCoord[];

extern const unsigned short INDICES[];
extern const unsigned int MAX_V_COUNT;
extern const unsigned int MAX_F_COUNT;

class GlesForm
	: public Tizen::Ui::Controls::Form
{
public:
	GlesForm(GlesShader* pApp)
		: __pApp(pApp)
	{
	}

	virtual ~GlesForm(void)
	{
	}

	virtual result OnDraw(void)
	{
		if (__pApp != null)
		{
			__pApp->Draw();
		}
		return E_SUCCESS;
	}

private:
	GlesShader* __pApp;
};

GlesShader::GlesShader(void)
	: __pForm(null)
	, __eglDisplay(EGL_NO_DISPLAY)
	, __eglSurface(EGL_NO_SURFACE)
	, __eglConfig(null)
	, __eglContext(EGL_NO_CONTEXT)
	, __programObject(0)
	, __idxVPosition(0)
	, __idxVNormal(0)
	, __idxVBO(0)
	, __idxIBO(0)
	, __idxLightDir(0)
	, __idxMVP(0)
	, __pTimer(null)
	, __idxTimeStamp(0)
	, __timeStamp(1)
	, __strideTimeStamp(0.02 * PI)
	, __stopCount(-1)
	, __angle(0.0f)
{
	__axis[0] = 0.0f;
	__axis[1] = 1.0f;
	__axis[2] = 0.0f;

	__byteBuffer = null;
	__URLDownLoaded = false;
	__TextureDownLoaded = false;
     state = 0;
}

GlesShader::~GlesShader(void)
{
}

void
GlesShader::Cleanup(void)
{
	if (__pTimer != null)
	{
		__pTimer->Cancel();
		delete __pTimer;
		__pTimer = null;
	}

	DestroyGL();
}

Application*
GlesShader::CreateInstance(void)
{
	// Create the instance through the constructor.
	return new (std::nothrow) GlesShader();
}

bool
GlesShader::OnAppInitializing(AppRegistry& appRegistry)
{
	result r = E_SUCCESS;

	Frame* pAppFrame = new (std::nothrow) Frame();
	TryReturn(pAppFrame != null, E_FAILURE, "[GlesShader] Generating a frame failed.");

	r = pAppFrame->Construct();
	TryReturn(!IsFailed(r), E_FAILURE, "[GlesShader] pAppFrame->Construct() failed.");

	this->AddFrame(*pAppFrame);

	__pForm = new (std::nothrow) GlesForm(this);
	TryCatch(__pForm != null, , "[GlesShader] Allocation of GlesForm failed.");

	r = __pForm->Construct(FORM_STYLE_NORMAL);
	TryCatch(!IsFailed(r), delete __pForm, "[GlesShader] __pForm->Construct(FORM_STYLE_NORMAL) failed.");

	r = GetAppFrame()->GetFrame()->AddControl(__pForm);
	TryCatch(!IsFailed(r), delete __pForm, "[GlesShader] GetAppFrame()->GetFrame()->AddControl(__pForm) failed.");

	__pForm->AddKeyEventListener(*this);

	TryCatch(InitEGL(), , "[GlesShader] GlesCube::InitEGL() failed.");

	TryCatch(InitGL(), , "[GlesShader] GlesCube::InitGL() failed.");

	GenerateVBO();
	Update();

	__pTimer = new (std::nothrow) Timer;
	TryCatch(__pTimer != null, , "[GlesShader] Failed to allocate memory.");

	r = __pTimer->Construct(*this);
	TryCatch(!IsFailed(r), , "[GlesShader] __pTimer->Construct(*this) failed.");

	return true;

CATCH:
	AppLog("[GlesShader] GlesShader::OnAppInitializing eglError : %#x\n"
			"[GlesShader] GlesShader::OnAppInitializing glError : %#x\n"
			"[GlesShader] GlesShader::OnAppInitializing VENDOR : %s\n"
			"[GlesShader] GlesShader::OnAppInitializing GL_RENDERER : %s\n"
			"[GlesShader] GlesShader::OnAppInitializing GL_VERSION : %s\n ",
			static_cast<unsigned int>(eglGetError()),
			static_cast<unsigned int>(glGetError()),
			glGetString(GL_VENDOR),
			glGetString(GL_RENDERER),
			glGetString(GL_VERSION));

	Cleanup();

	return false;
}

bool
GlesShader::OnAppTerminating(AppRegistry& appRegistry, bool forcedTermination)
{
	Cleanup();

	return true;
}

void
GlesShader::OnForeground(void)
{
	if (__pTimer != null)
	{
		__pTimer->Start(TIME_OUT);
	}
}

void
GlesShader::OnBackground(void)
{
	if (__pTimer != null)
	{
		__pTimer->Cancel();
	}
}

void
GlesShader::OnLowMemory(void)
{
}

void
GlesShader::OnBatteryLevelChanged(BatteryLevel batteryLevel)
{
}

void
GlesShader::OnScreenOn(void)
{
}

void
GlesShader::OnScreenOff(void)
{
}

void
GlesShader::OnTimerExpired(Timer& timer)
{
	if (__pTimer == null)
	{
		return;
	}
	__pTimer->Start(TIME_OUT);

	IncTimeStamp();

	Update();

	if (!Draw())
	{
		AppLog("[MeshLoader] MeshLoader::Draw() failed.");
	}
}

void
GlesShader::OnKeyPressed(const Control& source, Tizen::Ui::KeyCode keyCode)
{
}

void
GlesShader::OnKeyReleased(const Control& source, Tizen::Ui::KeyCode keyCode)
{
	if (keyCode == Tizen::Ui::KEY_BACK || keyCode == Tizen::Ui::KEY_ESC)
	{
		Terminate();
	}
}

void
GlesShader::OnKeyLongPressed(const Control& source, Tizen::Ui::KeyCode keyCode)
{
}

void
GlesShader::DestroyGL(void)
{
	DeleteVBO();

	if (__programObject)
	{
		glDeleteProgram(__programObject);
	}

	if (__eglDisplay)
	{
		eglMakeCurrent(__eglDisplay, null, null, null);

		if (__eglContext)
		{
			eglDestroyContext(__eglDisplay, __eglContext);
			__eglContext = EGL_NO_CONTEXT;
		}

		if (__eglSurface)
		{
			eglDestroySurface(__eglDisplay, __eglSurface);
			__eglSurface = EGL_NO_SURFACE;
		}

		eglTerminate(__eglDisplay);
		__eglDisplay = EGL_NO_DISPLAY;
	}

	return;
}

bool
GlesShader::InitEGL(void)
{
	EGLint numConfigs = 1;

	EGLint eglConfigList[] =
	{
		EGL_RED_SIZE,	8,
		EGL_GREEN_SIZE,	8,
		EGL_BLUE_SIZE,	8,
		EGL_ALPHA_SIZE,	8,
		EGL_DEPTH_SIZE, 24,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_NONE
	};

	EGLint eglContextList[] =
	{
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};

	eglBindAPI(EGL_OPENGL_ES_API);

	__eglDisplay = eglGetDisplay((EGLNativeDisplayType)EGL_DEFAULT_DISPLAY);
	TryCatch(__eglDisplay != EGL_NO_DISPLAY, , "[GlesShader] eglGetDisplay() failed.");

	TryCatch(!(eglInitialize(__eglDisplay, null, null) == EGL_FALSE || eglGetError() != EGL_SUCCESS), , "[GlesShader] eglInitialize() failed.");

	TryCatch(!(eglChooseConfig(__eglDisplay, eglConfigList, &__eglConfig, 1, &numConfigs) == EGL_FALSE ||
			eglGetError() != EGL_SUCCESS), , "[GlesShader] eglChooseConfig() failed.");

	TryCatch(numConfigs, , "[GlesShader] eglChooseConfig() failed. because of matching config doesn't exist");

	__eglSurface = eglCreateWindowSurface(__eglDisplay, __eglConfig, (EGLNativeWindowType)__pForm, null);
	TryCatch(!(__eglSurface == EGL_NO_SURFACE || eglGetError() != EGL_SUCCESS), , "[GlesShader] eglCreateWindowSurface() failed.");

	__eglContext = eglCreateContext(__eglDisplay, __eglConfig, EGL_NO_CONTEXT, eglContextList);
	TryCatch(!(__eglContext == EGL_NO_CONTEXT || eglGetError() != EGL_SUCCESS), , "[GlesShader] eglCreateContext() failed.");

	TryCatch(!(eglMakeCurrent(__eglDisplay, __eglSurface, __eglSurface, __eglContext) == EGL_FALSE ||
			eglGetError() != EGL_SUCCESS), , "[GlesShader] eglMakeCurrent() failed.");

	return true;

CATCH:
	{
		AppLog("[GlesShader] GlesShader can run on systems which supports OpenGL ES(R) 2.0.");
		AppLog("[GlesShader] When GlesShader does not correctly execute, there are a few reasons.");
		AppLog("[GlesShader]    1. The current device(real-target or emulator) does not support OpenGL ES(R) 2.0.\n"
				" Check the Release Notes.");
		AppLog("[GlesShader]    2. The system running on emulator cannot support OpenGL(R) 2.1 or later.\n"
				" Try with other system.");
		AppLog("[GlesShader]    3. The system running on emulator does not maintain the latest graphics driver.\n"
				" Update the graphics driver.");
	}

	DestroyGL();

	return false;
}

bool
GlesShader::InitGL(void)
{
	GLint linked = GL_FALSE;
	GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
	GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);

	{
		glShaderSource(fragShader, 1, static_cast<const char**> (&FRAGMENT_TEXT), null);
		glCompileShader(fragShader);
		GLint bShaderCompiled = GL_FALSE;
		glGetShaderiv(fragShader, GL_COMPILE_STATUS, &bShaderCompiled);

		TryCatch(bShaderCompiled != GL_FALSE, , "[GlesShader] Vertex shader program compile failed.");

		glShaderSource(vertShader, 1, static_cast<const char**> (&VERTEX_TEXT), null);
		glCompileShader(vertShader);
		glGetShaderiv(vertShader, GL_COMPILE_STATUS, &bShaderCompiled);

		TryCatch(bShaderCompiled != GL_FALSE, , "[GlesShader] Fragment shader program compile failed.");
	}

	__programObject = glCreateProgram();

	glAttachShader(__programObject, fragShader);
	glAttachShader(__programObject, vertShader);
	glLinkProgram(__programObject);
	glGetProgramiv(__programObject, GL_LINK_STATUS, &linked);

	if (linked == GL_FALSE)
	{
		GLint infoLen = 0;
		glGetProgramiv(__programObject, GL_INFO_LOG_LENGTH, &infoLen);

		if (infoLen > 1)
		{
			char* pInfoLog = new (std::nothrow) char[infoLen];

			glGetProgramInfoLog(__programObject, infoLen, null, pInfoLog);
			AppLog("[GlesShader] Linking failed. log: %s", pInfoLog);

			delete [] pInfoLog;
		}

		TryCatch(false, , "[GlesShader] linked == GL_FALSE");
	}

	__idxLightDir = glGetUniformLocation(__programObject, "u_lightDir");
	__idxMVP = glGetUniformLocation(__programObject, "u_mvpMatrix");
	__idxTimeStamp = glGetUniformLocation(__programObject, "u_timeStamp");

	__idxVPosition = glGetAttribLocation(__programObject, "a_position");
	__idxVNormal = glGetAttribLocation(__programObject, "a_normal");
	__idxVTexCoord =  glGetAttribLocation(__programObject, "a_texCoord");

	glUseProgram(__programObject);

	glEnable(GL_DEPTH_TEST);

	__lightDir[0] = 1.0 / Math::Sqrt(3);
	__lightDir[1] = 1.0 / Math::Sqrt(3);
	__lightDir[2] = -1.0 / Math::Sqrt(3);
	__lightDir[3] = 1.0;

	glDeleteShader(vertShader);
	glDeleteShader(fragShader);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	return true;

CATCH:
	glDeleteShader(vertShader);
	glDeleteShader(fragShader);

	return false;
}

void GlesShader::GenerateTexture()
{
	//------------------------------------------------
	String image_name = "tizen_icon.png"; 

	Tizen::Media::Image image;
	String filepath = App::GetInstance()->GetAppResourcePath() + image_name; 

	image.Construct();
	Bitmap *bitmap;

	if(state == 3) {

		File file;
		file.Construct(L"/tmp/" + __fileName, "wb");

		file.Write(*__byteBuffer);
		file.Flush();
                filepath = L"/tmp/" + __fileName;

		delete __byteBuffer;
	}

	bitmap = image.DecodeN(filepath, BITMAP_PIXEL_FORMAT_R8G8B8A8);

	BufferInfo info;
	bitmap->Lock(info);
	GLuint* pBitmapBuffer = (GLuint*)info.pPixels;

	glGenTextures(1, &__idxTexId);
	glBindTexture(GL_TEXTURE_2D, __idxTexId);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)(info.width), (GLsizei)(info.height), 0, GL_RGBA, GL_UNSIGNED_BYTE, pBitmapBuffer);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	bitmap->Unlock();

	delete pBitmapBuffer;
}

void
GlesShader::GenerateVBO(void)
{
	GenerateTexture();
}

void
GlesShader::DeleteVBO(void)
{
	glDeleteBuffers(1, &__idxVBO);
	glDeleteBuffers(1, &__idxIBO);
}

void
GlesShader::Update(void)
{
	if(state == 0)
	{
		__host    = L"http://fetchhell.nightsite.info";

		int r = rand();
		__fullURL = L"http://fetchhell.nightsite.info/tizen.txt?";
		__fullURL.Append(r);

		SendHttpRequest(__host, __fullURL);
	}

	if (state == 1)
	{ SendHttpRequest(__host, __fullURL); }

	if(state == 3) { GenerateTexture(); state = 0; }

	FloatMatrix4 matPerspective;
	FloatMatrix4 matModelview;

	const float EPSILON = 0.5f;

    __angle += 1.0;

	int x, y, width, height;
	__pForm->GetBounds(x, y, width, height);

	float aspect = float(width) / float(height);

	Perspective(&matPerspective, 60.0f, aspect, 1.0f, 20.0f);

	Translate(&matModelview, 0.0f, 0.0f, -2.5f);
	Rotate(&matModelview, __angle, __axis[0], __axis[1], __axis[2]);

	__matMVP = matPerspective * matModelview;
}


void
GlesShader::IncTimeStamp(void)
{
	__timeStamp += __strideTimeStamp;

	if (__timeStamp > 2 * PI)
	{
		__timeStamp = 0;
	}
}

bool
GlesShader::Draw(void)
{
	if (eglMakeCurrent(__eglDisplay, __eglSurface, __eglSurface, __eglContext) == EGL_FALSE ||
			eglGetError() != EGL_SUCCESS)
	{
		AppLog("[GlesShader] eglMakeCurrent() failed.");

		return false;
	}

	int x, y, width, height;
	__pForm->GetBounds(x, y, width, height);

	glViewport(0, 0, width, height);

	glEnable(GL_CULL_FACE);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUniform4fv(__idxLightDir, 1, __lightDir);
	glUniformMatrix4fv(__idxMVP, 1, GL_FALSE, (GLfloat*)__matMVP.matrix);
	glUniform1f(__idxTimeStamp, __timeStamp);

	glVertexAttribPointer(__idxVPosition, 3, GL_FLOAT, GL_FALSE, 0, VERTEX_1);
	glEnableVertexAttribArray(__idxVPosition);

	glVertexAttribPointer(__idxVTexCoord, 2, GL_FLOAT, GL_FALSE, 0, TexCoord);
	glEnableVertexAttribArray(__idxVTexCoord);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 24);

	eglSwapBuffers(__eglDisplay, __eglSurface);

	glDisableVertexAttribArray(__idxVPosition);
	glDisableVertexAttribArray(__idxVNormal);
	glDisableVertexAttribArray(__idxVTexCoord);

	return true;
}

void
GlesShader::Frustum(FloatMatrix4* pResult, float left, float right, float bottom, float top, float near, float far)
{
    float diffX = right - left;
    float diffY = top - bottom;
    float diffZ = far - near;

    if ((near <= 0.0f) || (far <= 0.0f) ||
        (diffX <= 0.0f) || (diffY <= 0.0f) || (diffZ <= 0.0f))
	{
    	return;
	}

    pResult->matrix[0][0] = 2.0f * near / diffX;
    pResult->matrix[1][1] = 2.0f * near / diffY;
    pResult->matrix[2][0] = (right + left) / diffX;
    pResult->matrix[2][1] = (top + bottom) / diffY;
    pResult->matrix[2][2] = -(near + far) / diffZ;
    pResult->matrix[2][3] = -1.0f;
    pResult->matrix[3][2] = -2.0f * near * far / diffZ;

    pResult->matrix[0][1] = pResult->matrix[0][2] = pResult->matrix[0][3] = 0.0f;
    pResult->matrix[1][0] = pResult->matrix[1][2] = pResult->matrix[1][3] = 0.0f;
    pResult->matrix[3][0] = pResult->matrix[3][1] = pResult->matrix[3][3] = 0.0f;
}

void
GlesShader::Perspective(FloatMatrix4* pResult, float fovY, float aspect, float near, float far)
{
	float fovRadian = fovY / 360.0f * PI;
	float top = tanf(fovRadian) * near;
	float right = top * aspect;

	Frustum(pResult, -right, right, -top, top, near, far);
}

void
GlesShader::Translate(FloatMatrix4* pResult, float tx, float ty, float tz)
{
	pResult->matrix[3][0] += (pResult->matrix[0][0] * tx + pResult->matrix[1][0] * ty + pResult->matrix[2][0] * tz);
	pResult->matrix[3][1] += (pResult->matrix[0][1] * tx + pResult->matrix[1][1] * ty + pResult->matrix[2][1] * tz);
    pResult->matrix[3][2] += (pResult->matrix[0][2] * tx + pResult->matrix[1][2] * ty + pResult->matrix[2][2] * tz);
    pResult->matrix[3][3] += (pResult->matrix[0][3] * tx + pResult->matrix[1][3] * ty + pResult->matrix[2][3] * tz);
}

void
GlesShader::Rotate(FloatMatrix4* pResult, float angle, float x, float y, float z)
{
	FloatMatrix4 rotate;

	float cos = cosf(angle * PI / 180.0f);
	float sin = sinf(angle * PI / 180.0f);
	float cos1 = 1.0f - cos;

	FloatVector4 vector(x, y, z, 0.0f);
	vector.Normalize();
	x = vector.x;
	y = vector.y;
	z = vector.z;

	rotate.matrix[0][0] = (x * x) * cos1 + cos;
	rotate.matrix[0][1] = (x * y) * cos1 - z * sin;
	rotate.matrix[0][2] = (z * x) * cos1 + y * sin;
	rotate.matrix[0][3] = 0.0f;

	rotate.matrix[1][0] = (x * y) * cos1 + z * sin;
	rotate.matrix[1][1] = (y * y) * cos1 + cos;
	rotate.matrix[1][2] = (y * z) * cos1 - x * sin;
	rotate.matrix[1][3] = 0.0f;

	rotate.matrix[2][0] = (z * x) * cos1 - y * sin;
	rotate.matrix[2][1] = (y * z) * cos1 + x * sin;
	rotate.matrix[2][2] = (z * z) * cos1 + cos;

	rotate.matrix[2][3] = rotate.matrix[3][0] = rotate.matrix[3][1] = rotate.matrix[3][2] = 0.0f;
	rotate.matrix[3][3] = 1.0f;

	*pResult *= rotate;
}

// Work with NET
//------------------------------------------------
result GlesShader::SendHttpRequest(String host, String fullURL)
{
	if (state == 1) state = 2;
	if(state == 0) state = 5;

    result r = E_SUCCESS;

     AppLog("%ls", fullURL.GetPointer());

    __httpSession = new (std::nothrow) HttpSession();
    r = __httpSession->Construct(NET_HTTP_SESSION_MODE_NORMAL, null, host, null);

    __httpTransaction = __httpSession->OpenTransactionN();
    r = __httpTransaction->AddHttpTransactionListener(*this);

    HttpRequest* httpRequest = __httpTransaction->GetRequest();
    NetHttpMethod method = NET_HTTP_METHOD_GET;
    httpRequest->SetMethod(method);

    httpRequest->SetUri(fullURL);
    HttpHeader* httpHeader = httpRequest->GetHeader();

    r = __httpTransaction->Submit();

    return r;
}

//-------------------------------------------------------
void GlesShader::OnTransactionCompleted(HttpSession& httpSession, HttpTransaction& httpTransaction)
{
	ByteBuffer* pBody = null;
	HttpResponse* httpResponse = __httpTransaction->GetResponse();
	if (httpResponse->GetHttpStatusCode() == HTTP_STATUS_OK) {

		    __byteBuffer = httpResponse->ReadBodyN();

			if(state==5)
			{
				Encoding* pEnc = Encoding::GetEncodingN(L"ISO-8859-1");

				String str;
				pEnc->GetString(*__byteBuffer, str);

				__fullURL = __host + "/" + str;
				__fileName = str;

				AppLog("%ls", str.GetPointer());
				state = 1;

				delete pEnc;
			}
			else if (state == 2)
			{
				state = 3;
			}

			delete __httpSession;
			delete __httpTransaction;
	}
}



