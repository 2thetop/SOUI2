// dui-demo.cpp : main source file
//

#include "stdafx.h"

#include <unknown/com-loader.hpp>

#include "MainDlg.h"

//#define RENDER_GDI      //打开RENDER_GDI时使用render-gdi模块来渲染，否则采用render-skia渲染

#define SUPPORT_LANG    //打开SUPPORT_LANG时，演示多语言支持

#define RES_TYPE 0   //从文件中加载资源
// #define RES_TYPE 1   //从PE资源中加载UI资源

#ifdef _DEBUG
#define COM_IMGDECODER  _T("imgdecoder-wicd.dll")
#define COM_RENDER_GDI  _T("render-gdid.dll")
#define COM_RENDER_SKIA _T("render-skiad.dll")
#define COM_SCRIPT_LUA _T("scriptmodule-luad.dll")
#define COM_TRANSLATOR _T("translatord.dll")
#define SYS_NAMED_RESOURCE _T("soui-sys-resourced.dll")
#else
#define COM_IMGDECODER  _T("imgdecoder-wic.dll")
#define COM_RENDER_GDI  _T("render-gdi.dll")
#define COM_RENDER_SKIA _T("render-skia.dll")
#define COM_SCRIPT_LUA _T("scriptmodule-lua.dll")
#define COM_TRANSLATOR _T("translator.dll")
#define SYS_NAMED_RESOURCE _T("soui-sys-resource.dll")
#endif


int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR /*lpstrCmdLine*/, int /*nCmdShow*/)
{
    HRESULT hRes = OleInitialize(NULL);
    ASSERT(SUCCEEDED(hRes));
    
    int nRet = 0; 

    SComLoader imgDecLoader;
    SComLoader renderLoader;
    SComLoader transLoader;
    
    //将程序的运行路径修改到项目所在目录所在的目录
    TCHAR szCurrentDir[MAX_PATH]={0};
    GetModuleFileName( NULL, szCurrentDir, sizeof(szCurrentDir) );
    LPTSTR lpInsertPos = _tcsrchr( szCurrentDir, _T('\\') );
    _tcscpy(lpInsertPos+1,_T(".."));
    SetCurrentDirectory(szCurrentDir);
        
    {

        CAutoRefPtr<SOUI::IImgDecoderFactory> pImgDecoderFactory;
        CAutoRefPtr<SOUI::IRenderFactory> pRenderFactory;
        imgDecLoader.CreateInstance(COM_IMGDECODER,(IObjRef**)&pImgDecoderFactory);
#ifdef RENDER_GDI
        renderLoader.CreateInstance(COM_RENDER_GDI,(IObjRef**)&pRenderFactory);
#else
        renderLoader.CreateInstance(COM_RENDER_SKIA,(IObjRef**)&pRenderFactory);
#endif

        pRenderFactory->SetImgDecoderFactory(pImgDecoderFactory);

        SApplication *theApp=new SApplication(pRenderFactory,hInstance);

        HMODULE hSysResource=LoadLibrary(SYS_NAMED_RESOURCE);
        if(hSysResource)
        {
            CAutoRefPtr<IResProvider> sysSesProvider;
            CreateResProvider(RES_PE,(IObjRef**)&sysSesProvider);
            sysSesProvider->Init((WPARAM)hSysResource,0);
            theApp->LoadSystemNamedResource(sysSesProvider);
        }

        CAutoRefPtr<ITranslatorMgr> trans;
        transLoader.CreateInstance(COM_TRANSLATOR,(IObjRef**)&trans);

        if(trans)
        {
            theApp->SetTranslator(trans);
            pugi::xml_document xmlLang;
            if(xmlLang.load_file(L"Translator/lang_cn.xml"))
            {
                CAutoRefPtr<ITranslator> langCN;
                trans->CreateTranslator(&langCN);
                langCN->Load(&xmlLang.child(L"language"),1);//1=LD_XML
                trans->InstallTranslator(langCN);
            }
        }
        CAutoRefPtr<IResProvider>   pResProvider;
#if (RES_TYPE == 0)
        CreateResProvider(RES_FILE,(IObjRef**)&pResProvider);
        if(!pResProvider->Init((LPARAM)_T("uires"),0))
        {
            ASSERT(0);
            return 1;
        }
#else 
        CreateResProvider(RES_PE,(IObjRef**)&pResProvider);
        pResProvider->Init((WPARAM)hInstance,0);
#endif

        theApp->AddResProvider(pResProvider);
        

        theApp->Init(_T("XML_INIT")); 

        // BLOCK: Run application
        {
            CMainDlg dlgMain;  
            dlgMain.Create(GetActiveWindow(),0,0,800,600);
            dlgMain.SendMessage(WM_INITDIALOG);
            dlgMain.CenterWindow(dlgMain.m_hWnd);
            dlgMain.ShowWindow(SW_SHOWNORMAL);
            nRet=theApp->Run(dlgMain.m_hWnd);
        }

        delete theApp;
    }

    OleUninitialize();
    return nRet;
}
