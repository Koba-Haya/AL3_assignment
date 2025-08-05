#include "GameScene.h"
#include "TitleScene.h"
#include <KamataEngine.h>
#include <Windows.h>

using namespace KamataEngine;

GameScene* gameScene = nullptr;
TitleScene* titleScene = nullptr;

// シーン（型）
enum class Scene {
	kUnknown = 0,
	kTitle,
	kGame,
};

// 現在シーン（型）
Scene scene = Scene::kUnknown;

void ChangeScene();
void UpdateScene();
void DrawScene();

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

	// エンジンの初期化
	KamataEngine::Initialize(L"LE2B_10_コバヤシ_ハヤト_AL3");

	// DirectXCommonインスタンスの取得
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	scene = Scene::kTitle;
	titleScene = new TitleScene;

	// メインループ
	while (true) {
		// エンジンの更新
		if (KamataEngine::Update()) {
			break;
		}

		// 描画開始
		dxCommon->PreDraw();

		KamataEngine::Model::PreDraw(dxCommon->GetCommandList());

		ChangeScene();
		UpdateScene();
		DrawScene();

		KamataEngine::Model::PostDraw();

		// 描画終了
		dxCommon->PostDraw();
	}
	delete titleScene;
	delete gameScene;

	// エンジンの終了処理
	KamataEngine::Finalize();

	return 0;
}

void ChangeScene() {
	switch (scene) {
	case Scene::kUnknown:
		break;
	case Scene::kTitle:
		if (titleScene->IsFinished()) {
		// シーン変更
			scene = Scene::kGame;
			// 旧シーンの開放
			delete titleScene;
			titleScene = nullptr;
			// 新シーンの生成と初期化
			gameScene = new GameScene;
			gameScene->Initialize();
		}
		break;
	case Scene::kGame:
		if (gameScene->IsFinished()) {
			// シーン変更
			scene = Scene::kTitle;
			// 旧シーンの開放
			delete gameScene;
			gameScene = nullptr;
			// 新シーンの生成と初期化
			titleScene = new TitleScene;
			titleScene->Initialize();
		}
		break;
	default:
		break;
	}
}

void UpdateScene() {
	switch (scene) {
	case Scene::kUnknown:
		break;
	case Scene::kTitle:
		titleScene->Update();
		break;
	case Scene::kGame:
		gameScene->Update();
		break;
	default:
		break;
	}
}

void DrawScene() {
	switch (scene) {
	case Scene::kUnknown:
		break;
	case Scene::kTitle:
		titleScene->Draw();
		break;
	case Scene::kGame:
		gameScene->Draw();
		break;
	default:
		break;
	}
}
