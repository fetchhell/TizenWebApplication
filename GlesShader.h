#ifndef _GLES_SHADER_H_
#define _GLES_SHADER_H_


#include <new>
#include <FApp.h>
#include <FBase.h>
#include <FSystem.h>
#include <FMedia.h>
#include <FUi.h>
#include <FGraphics.h>
#include <FGraphicsOpengl2.h>
#include <FGrpFloatMatrix4.h>


#include <FBase.h>
#include <FNet.h>
#include <FBaseLog.h>
#include <FText.h>
#include <FIo.h>

using namespace Tizen::Base;
using namespace Tizen::Net::Http;
using namespace Tizen::Text;
using namespace Tizen::Io;
using namespace Tizen::App;

class GlesShader
	: public Tizen::App::Application
	, public Tizen::Base::Runtime::ITimerEventListener
	, public Tizen::Ui::IKeyEventListener
	, public Tizen::Net::Http::IHttpTransactionEventListener
{
public:
	static Tizen::App::Application* CreateInstance(void);

public:
	GlesShader();
	~GlesShader();

	virtual bool OnAppInitializing(Tizen::App::AppRegistry& appRegistry);
	virtual bool OnAppTerminating(Tizen::App::AppRegistry& appRegistry, bool forcedTermination = false);
	virtual void OnForeground(void);
	virtual void OnBackground(void);
	virtual void OnLowMemory(void);
	virtual void OnBatteryLevelChanged(Tizen::System::BatteryLevel batteryLevel);
	virtual void OnScreenOn(void);
	virtual void OnScreenOff(void);

	virtual void OnTimerExpired(Tizen::Base::Runtime::Timer& timer);

	virtual void OnKeyPressed(const Tizen::Ui::Control& source, Tizen::Ui::KeyCode keyCode);
	virtual void OnKeyReleased(const Tizen::Ui::Control& source, Tizen::Ui::KeyCode keyCode);
	virtual void OnKeyLongPressed(const Tizen::Ui::Control& source, Tizen::Ui::KeyCode keyCode);

	bool Draw(void);

	result SendHttpRequest(String host, String fullURL);

private:
	bool InitEGL(void);
	bool InitGL(void);
	void DestroyGL(void);
	void Update(void);
	void Cleanup(void);

	void Frustum(Tizen::Graphics::FloatMatrix4* result, float left, float right, float bottom, float top, float nearZ, float farZ);
	void Perspective(Tizen::Graphics::FloatMatrix4* result, float fovY, float aspect, float nearZ, float farZ);
	void Translate(Tizen::Graphics::FloatMatrix4* result, float tx, float ty, float tz);
	void Rotate(Tizen::Graphics::FloatMatrix4* pResult, float angle, float x, float y, float z);

	void GenerateVBO(void);
	void DeleteVBO(void);

	void GenerateTexture(void);

	void IncTimeStamp(void);

private:

	void OnTransactionReadyToRead(Tizen::Net::Http::HttpSession &httpSession, Tizen::Net::Http::HttpTransaction &httpTransaction,
			                      int recommendedChunkSize){};

	void OnTransactionCertVerificationRequiredN(HttpSession& httpSession, HttpTransaction& httpTransaction,
			Tizen::Base::String* pCert){};

	void OnTransactionCompleted(HttpSession& httpSession, HttpTransaction& httpTransaction);

	void OnTransactionHeaderCompleted(HttpSession& httpSession, HttpTransaction& httpTransaction,
			int headerLen, bool bAuthRequired){};

	void OnTransactionReadyToWrite(HttpSession& httpSession, HttpTransaction& httpTransaction, int recommendedChunkSize){};

	void OnTransactionAborted(HttpSession& httpSession, HttpTransaction& httpTransaction, result r){};

	Tizen::Ui::Controls::Form*			__pForm;

	Tizen::Graphics::Opengl::EGLDisplay	__eglDisplay;
	Tizen::Graphics::Opengl::EGLSurface	__eglSurface;
	Tizen::Graphics::Opengl::EGLConfig	__eglConfig;
	Tizen::Graphics::Opengl::EGLContext	__eglContext;
	Tizen::Graphics::Opengl::GLuint		__programObject;
	Tizen::Graphics::Opengl::GLuint		__idxVPosition;
	Tizen::Graphics::Opengl::GLuint		__idxVTexCoord;
	Tizen::Graphics::Opengl::GLuint		__idxVNormal;
	Tizen::Graphics::Opengl::GLuint		__idxVBO;
	Tizen::Graphics::Opengl::GLuint		__idxIBO;
	Tizen::Graphics::Opengl::GLint		__idxLightDir;
	Tizen::Graphics::Opengl::GLint		__idxMVP;

	Tizen::Graphics::FloatMatrix4		__matMVP;
	Tizen::Graphics::Opengl::GLfloat 		__lightDir[4];

	Tizen::Base::Runtime::Timer*			__pTimer;
	Tizen::Graphics::Opengl::GLint		__idxTimeStamp;
	Tizen::Graphics::Opengl::GLfloat		__timeStamp, __strideTimeStamp;

	Tizen::Graphics::Opengl::GLuint     __idxTexId;

	int 								__stopCount;
	float 								__angle, __axis[3];

	String 								__fullURL;
	String								__host;
	String 								__fileName;
	bool 								__URLDownLoaded;
	bool								__TextureDownLoaded;
	int                                   state;

	//--------------------------------------------------
	Tizen::Net::Http::HttpTransaction *__httpTransaction;
	Tizen::Net::Http::HttpSession *__httpSession;

	ByteBuffer *__byteBuffer;
};

#endif

