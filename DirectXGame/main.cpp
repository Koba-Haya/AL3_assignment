#include "GameScene.h"
#include <KamataEngine.h>
#include <Windows.h>

using namespace KamataEngine;

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

	// エンジンの初期化
	KamataEngine::Initialize(L"LE2B_10_コバヤシ_ハヤト_AL3");

	// DirectXCommonインスタンスの取得
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	// ゲームシーンのインスタンス生成
	GameScene* gamescene = new GameScene();
	// ゲームシーンの初期化
	gamescene->Initialize();

	// メインループ
	while (true) {
		// エンジンの更新
		if (KamataEngine::Update()) {
			break;
		}
		// ゲームシーンの更新
		gamescene->Update();

		// 描画開始
		dxCommon->PreDraw();

		//=================================
		//          ここに描画処理
		//=================================

		// 描画終了
		dxCommon->PostDraw();
	}
	// ゲームシーンの開放
	delete gamescene;
	// nullptrの代入
	gamescene = nullptr;

	// エンジンの終了処理
	KamataEngine::Finalize();

	return 0;
}
