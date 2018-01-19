#include <stdio.h>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

#include "editline/readline.h"
#include "ncbind/ncbind.hpp"

#include "license.c"

ttstr
convertUtf8StringToTtstr(const char *buf, size_t length)
{
	tjs_uint maxlen = (tjs_uint)length * 2 + 1;
	tjs_char *dat = new tjs_char[maxlen];
	tjs_uint len = TVPUtf8ToWideCharString(buf, dat);
	ttstr result(dat, len);
	delete[] dat;
	return result;
}


class MyConsole : public tTVPContinuousEventCallbackIntf 
{

public:
	/**
	 * コンストラクタ
	 */
	MyConsole() : abort_(false), th_(&MyConsole::run, this) { 
	}

	/**
	 * デストラクタ
	 */
	virtual ~MyConsole() {
		abort_thread();
		th_.join();
	}

	/**
 	 * スレッド処理を中断させる
	 */
	void abort_thread() {
		std::lock_guard<std::mutex> lk(mtx_);
		if (!abort_) {
			abort_ = true;
			// 標準入力の強制破棄
			cv_.notify_all();
		}
	}

	virtual void TJS_INTF_METHOD OnContinuousCallback(tjs_uint64 tick) {
		if (line.length() > 0) {
			std::unique_lock<std::mutex> lk(mtx_);
			TVPDoTryBlock(TryExec, Catch, Finally, (void *)this);
			line.Clear();
		}
	}

private:
	ttstr line;
	bool abort_;
    std::mutex mtx_;
    std::condition_variable cv_;

    // threadオブジェクトは最後に記述が必要
    std::thread th_;

	/**
	 * コンソール処理本体
	 */
	void run() {
        const auto sleep_time = std::chrono::milliseconds(5);
		const char* file = ".history";
		char const* prompt = "> ";

		read_history(file);

		while (true) {
			std::unique_lock<std::mutex> lk(mtx_);

            if (abort_) {
                break;
            }

			if (line.length() == 0) {
				// 入力データが無い場合に入力処理
				char* result = readline(prompt);
				if (result == NULL) {
					break;
				} 
				if (*result != '\0') {
					line = convertUtf8StringToTtstr(result, strlen(result));
					add_history(result);
				}
				rl_free(result);
			}
			// 中断つき待機
			cv_.wait_for(lk, sleep_time, [this] { return abort_; });
		}
		write_history(file);
		free_history();
	}

	static void outputStr(const ttstr &msg) {
		DWORD n_chars;
		HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
		::WriteConsoleW(handle, msg.c_str(), msg.length(), &n_chars, 0);
		::WriteConsoleW(handle, L"\n", 1, &n_chars, 0);
	}

	static void TJS_USERENTRY TryExec(void * data) {
		MyConsole *self = (MyConsole*)data;
		tTJSVariant result;
		TVPExecuteExpression(self->line, &result);
		result.ToString();
		ttstr str = result;
		outputStr(str);
	}

	static bool TJS_USERENTRY Catch(void * data, const tTVPExceptionDesc & desc) {
		outputStr(desc.message);
		//例外は無視
		return false;
	}

	static void TJS_USERENTRY Finally(void * data) {
	}
};

static MyConsole *console;

void PreRegisterCallback()
{
	TVPAddImportantLog(ttstr("------ wineditline Copyright START ------"));
	TVPAddImportantLog(ttstr(license_text));
	TVPAddImportantLog(ttstr("------ wineditline Copyright END ------"));

	if (::AttachConsole(-1) || ::GetLastError() == ERROR_ACCESS_DENIED) {
		TVPAddImportantLog("attach console success");
		console = new MyConsole();
	} else {
		TVPAddImportantLog("attach console failed");
	}
	if (console) {
		TVPAddContinuousEventHook(console);
	}
}

void PostUnRegisterCallback()
{
	if (console) {
		TVPRemoveContinuousEventHook(console);
		::FreeConsole();
		delete console;
		console = 0;
	}
}

NCB_PRE_REGIST_CALLBACK(PreRegisterCallback);
NCB_POST_UNREGIST_CALLBACK(PostUnRegisterCallback);
