// Oyun ekrani: denge bari (+ yerlestirme tahmini), egitim ipucu kutusu, ailenin konusma balonu,
// gorev listesi, sagda bina cekmecesi, uyari/bolum mesaji. H ile gelistirici test paneli.
// Yigit'in WBP_HUD'u gelince bu sinif yalnizca test paneline iner; okudugu degerler BlueprintReadOnly.
// Metinde emoji yok (04_UNREAL_TUZAKLARI: fontta olmayan glif oyunu donduruyor).

#pragma once

#include "CoreMinimal.h"
#include "Engine/Canvas.h"
#include "GameFramework/HUD.h"
#include "SLHUD.generated.h"

class ASLAmbience;
class ASLLevelDirector;
class ASLPlatform;
class USLBuildComponent;
struct FSLSlot;

enum class ESLMenuPage : uint8 { Main, Levels, HowTo, Credits };

UCLASS()
class SALLANTI_API ASLHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	/** Takim adi: menu altinda ve jenerikte. Teslimden once Yigit yazar. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Jenerik")
	FString TeamName = TEXT("[Takım adı]");

	/** Jenerikteki ekip satirlari. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Jenerik")
	TArray<FString> TeamMembers = {
		TEXT("Andaç · programlama, oyun tasarımı"),
		TEXT("Yiğit · arayüz, seviye, paketleme"),
		TEXT("Mert · model, doku, ışık"),
		TEXT("Asil · konsept, model") };

	/** Menu: Esc bir ust sayfaya. */
	void MenuBack();
	void DoMenuAction(int32 Action);

	/** Trailer cekimi: ortada baslik karti (alfa 0..1) ve altta altyazi. */
	FString CinemaTitle;
	FString CinemaSub;
	float CinemaTitleAlpha = 0.f;
	FString CinemaCaption;
	bool bCinema = false;

	/** Gelistirici paneli (sayilar, CoM, ayarlar). Oyuncu ekrani her zaman cizilir. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|HUD")
	bool bShowTestPanel = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|HUD")
	float TextScale = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|HUD")
	float BalanceBarWidth = 520.f;

	/** Cekmece genisligi, px. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Cekmece")
	float DrawerWidth = 330.f;

	/** Cekmece acilip kapanma hizi (1/sn). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Cekmece")
	float DrawerSpeed = 6.f;

	/** Kart secilince cekmece kapansin mi (platform gorunsun). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sallanti|Cekmece")
	bool bCloseDrawerOnSelect = true;

	/** Fare basildi: HUD'a dustuyse (cekmece) true doner, oyun tiklamayi islemez. */
	bool HandleMouseDown(const FVector2D& Mouse);
	void HandleMouseUp(const FVector2D& Mouse);

	void ToggleDrawer() { SetDrawerOpen(!bDrawerWanted); }
	void SetDrawerOpen(bool bOpen);
	bool IsDrawerOpen() const { return DrawerOpen > 0.5f; }
	bool IsDrawerWanted() const { return bDrawerWanted; }

	/** Test: cekmecedeki I. kartin ekran merkezi (son cizimden). */
	FVector2D GetCardCenter(int32 SlotIndex) const
	{
		const int32 K = CardSlots.IndexOfByKey(SlotIndex);
		return CardRects.IsValidIndex(K) ? CardRects[K].GetCenter() : FVector2D::ZeroVector;
	}
	FVector2D GetHandleCenter() const { return HandleRect.GetCenter(); }

	/** Su an gosterilen egitim ipucu (test ve log icin). */
	const FString& GetCurrentHint() const { return LastHint; }

	/** Kisa sureli ipucu (ornegin kilitli karta tiklayinca). */
	void Flash(const FString& Text, float Seconds = 2.f);

private:
	void DrawBalanceBar(const ASLPlatform& P, const USLBuildComponent& B);
	void DrawHint(const ASLPlatform& P, const USLBuildComponent& B, const ASLLevelDirector* D);
	void DrawSpeech(const ASLLevelDirector& D);
	void DrawTasks(const ASLLevelDirector& D);
	/** Sol: gorev kartinin altinda aileler ve mutluluk barlari (bolumde istek varsa). */
	void DrawFamilies(const ASLLevelDirector& D);
	/** Evlerin ustunde mutluluk yuzu (dunyaya bagli). */
	void DrawHomeFaces(const ASLLevelDirector& D);
	void DrawFace(float CX, float CY, float R, float Happy01);
	/** Aileler: ustlerinde "?" (istekleri bilinmiyor), konusma balonlari, secilinin istek karti. */
	void DrawFamilyOverlays(ASLLevelDirector& D);
	void DrawBubble(const FString& S, const FVector& WorldAnchor, float MaxW, float Scale, const FLinearColor& Fill);
	void DrawWishCard(const ASLLevelDirector& D, int32 Fam);
	int32 FamilyUnderMouse(const ASLLevelDirector& D) const;
	/** Sag ust: gun, saat (gunes/ay), kalan hamle, muzik dugmesi. */
	void DrawClock(const ASLLevelDirector& D);
	void DrawSpeakerIcon(float CX, float CY, float S, bool bOn, const FLinearColor& C);

	void DrawDrawer(const USLBuildComponent& B, float DeltaSeconds);
	void DrawCenterMessage(const ASLPlatform& P, bool bLevelComplete);
	void DrawTestPanel(const ASLPlatform& P, const USLBuildComponent& B);

	/** Egitim ipucunu ve rengini secer; durum sirasi: devrilme > hayalet > uyari > gorev. */
	void BuildHint(const ASLPlatform& P, const USLBuildComponent& B, const ASLLevelDirector* D, FString& Out, FLinearColor& Color) const;
	/** Elindekini koyarsan olacak egim (yoksa negatif). */
	float PredictPlacement(const ASLPlatform& P, const USLBuildComponent& B) const;

	void FillRound(float X, float Y, float W, float H, float R, const FLinearColor& C);
	void FillCircle(float CX, float CY, float R, const FLinearColor& C);
	void FillTri(float CX, float ApexY, float BaseW, float H, const FLinearColor& C);
	/** Golgeli yuvarlak kart. */
	void Card(float X, float Y, float W, float H, float R, const FLinearColor& C);
	void DrawHouseIcon(float CX, float CY, float S, const FLinearColor& C);
	void DrawLockIcon(float CX, float CY, float S, const FLinearColor& C);
	void DrawSlotThumb(const FSLSlot& S, float X, float Y, float Size, bool bOpen);
	bool bTextShadow = false;

	void Text(const FString& S, float X, float Y, const FLinearColor& Color, float Scale = 1.f, bool bLarge = false);
	void Line(float X, float& Y, const FString& S, const FLinearColor& Color = FLinearColor::White);
	FVector2D Measure(const FString& S, float Scale, bool bLarge);
	TArray<FString> Wrap(const FString& S, float MaxWidth, float Scale);

	struct FRect
	{
		float X = 0.f, Y = 0.f, W = 0.f, H = 0.f;
		bool Contains(const FVector2D& P) const { return P.X >= X && P.X <= X + W && P.Y >= Y && P.Y <= Y + H; }
		FVector2D GetCenter() const { return FVector2D(X + 0.5f * W, Y + 0.5f * H); }
	};

	/** Fare bu dikdortgenin ustunde mi; ustundeyse imlec el olur. */
	bool Hover(const FRect& R);

	// --- Gokyuzu pusulasi (C yonu) ---
	float UiScale() const { return Canvas ? FMath::Max(0.6f, Canvas->ClipY / 800.f) : 1.f; }
	void Arc(float CX, float CY, float R, float A0, float A1, float Thick, const FLinearColor& C, bool bDashed = false);
	float PillAlpha(float Day) const;
	void TextHalo(const FString& S, float X, float Y, const FLinearColor& C, float Scale);
	void TextSpaced(const FString& S, float CenterX, float Y, float Scale, float Spacing, const FLinearColor& C);
	void OutlineRound(float X, float Y, float W, float H, float R, float Thick, const FLinearColor& Line, const FLinearColor& Fill);
	void DrawSky(const ASLLevelDirector& D);
	void DrawConstellation(ASLLevelDirector& D);
	void DrawLens(const ASLLevelDirector& D, int32 Fam);
	void DrawCompass(const ASLPlatform& P, const USLBuildComponent& B);
	void DrawRotateButtons();
	void DrawHintC(const ASLPlatform& P, const USLBuildComponent& B, const ASLLevelDirector* D);
	void DrawEndC(const ASLPlatform& P, bool bLevelComplete);
	void DrawHomeNodes(const ASLLevelDirector& D);
	FRect RotLeftRect;
	FRect RotRightRect;
	TArray<FRect> FamilyRowRects;

	// --- Menu ve jenerik ---
	TArray<FString> GetCreditsLines() const;
	bool MenuButton(const FString& Label, float X, float Y, float Scale, FRect& Out);
	void DrawMenu();
	void DrawCredits();
	void DrawCinemaOverlay();
	ESLMenuPage MenuPage = ESLMenuPage::Main;
	TArray<FRect> MenuRects;
	TArray<int32> MenuActions;
	float CreditsStart = 0.f;
	bool bCreditsChecked = false;
	int32 NextAction = 0; // bitis ekraninin ikinci dugmesi: 0 sonraki bolum, 21 menu, 22 jenerik

	float DrawerOpen = 0.f;  // 0 kapali, 1 acik (cizim)
	bool bDrawerWanted = false;
	bool bDragging = false;
	float DragStartMouseX = 0.f;
	float DragStartOpen = 0.f;
	bool bDragMoved = false;
	FRect HandleRect;
	FRect DrawerRect;
	TArray<FRect> CardRects;
	TArray<int32> CardSlots; // kart -> slot indeksi (bolumde kullanilmayan slotlar gizli)

	// --- Etkilesim (16 Eki): ustune gelince parlama, basinca kuculme, kilitliye tiklayinca sallanma ---
	FVector2D MousePos = FVector2D::ZeroVector;
	bool bAnyHover = false;
	float NowReal = 0.f;
	int32 PressedCard = -1;
	float PressedAt = -10.f;
	int32 ShakeCard = -1;
	float ShakeAt = -10.f;
	FRect HeaderCloseRect;
	FRect MusicRect;
	FRect AgainRect;
	FRect NextRect;
	TWeakObjectPtr<ASLAmbience> Ambience;
	float TasksBottom = 0.f;
	float LastDrawTime = -1.f;

	FString FlashText;
	float FlashUntil = -1.f;
	FString LastHint;
};
