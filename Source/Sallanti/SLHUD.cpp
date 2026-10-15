#include "SLHUD.h"

#include "SLAmbience.h"
#include "EngineUtils.h"
#include "SLBuildComponent.h"
#include "SLCameraPawn.h"
#include "SLFamily.h"
#include "SLGameMode.h"
#include "SLLevelDirector.h"
#include "SLPlaceable.h"
#include "SLPlatform.h"
#include "SLPlayerController.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"

namespace
{
	// Cozy palet (14 Eki, referans gorsel): krem kartlar, turkuaz / sari / mercan durum renkleri.
	// Canvas renkleri ekranda oldugu gibi (sRGB'ye yakin) cikiyor; olculdu.
	const FLinearColor Teal(0.25f, 0.70f, 0.66f);
	const FLinearColor Yellow(1.f, 0.76f, 0.30f);
	const FLinearColor Red(0.92f, 0.40f, 0.38f);
	const FLinearColor Green = Teal;
	const FLinearColor Info = Teal;
	const FLinearColor Panel(0.f, 0.f, 0.f, 0.5f); // yalniz test paneli
	const FLinearColor Dim(1.f, 1.f, 1.f, 0.75f);
	const FLinearColor PanelColor(0.99f, 0.97f, 0.93f, 0.97f);
	const FLinearColor Ink(0.27f, 0.22f, 0.25f);
	const FLinearColor Muted(0.55f, 0.50f, 0.53f);
	constexpr float HandleW = 36.f;
	constexpr float HandleH = 170.f;
	constexpr float CardH = 86.f;

	FLinearColor Pale(const FLinearColor& C) { return FMath::Lerp(C, FLinearColor::White, 0.6f); }
}

// --- Yardimcilar --------------------------------------------------------------------------------

UFont* ASLHUD::MakeRuntimeFont(const FString& FileName)
{
	// Content/Fonts/<ad>.ttf (OFL; pakete NonUFS olarak kopyalanir, DefaultGame.ini). Font aktarma
	// fabrikasi bassiz modda onay penceresi acip cokuyordu: UFont oyun icinde dosyadan kurulur.
	const FString Path = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("Fonts") / FileName);
	if (!FPaths::FileExists(Path))
	{
		UE_LOG(LogTemp, Warning, TEXT("SL_FONT yok: %s (Roboto kullaniliyor)"), *Path);
		return nullptr;
	}
	UFont* F = NewObject<UFont>(this);
	F->FontCacheType = EFontCacheType::Runtime;
	F->LegacyFontSize = 24;
	F->CompositeFont.DefaultTypeface.Fonts.Add(FTypefaceEntry(TEXT("Regular"), Path, EFontHinting::Default, EFontLoadingPolicy::LazyLoad));
	UE_LOG(LogTemp, Log, TEXT("SL_FONT %s"), *Path);
	return F;
}

UFont* ASLHUD::PickFont(bool bLarge, float& OutMul)
{
	UFont* Roboto = GEngine ? (bLarge ? GEngine->GetLargeFont() : GEngine->GetMediumFont()) : nullptr;
	OutMul = 1.f;
	if (!bFontsInit)
	{
		bFontsInit = true;
		TitleFont = MakeRuntimeFont(TEXT("Cinzel.ttf"));
		BodyFont = MakeRuntimeFont(TEXT("Jost.ttf"));
	}
	UFont* F = bUseTitleFont && TitleFont ? TitleFont.Get() : BodyFont.Get();
	if (!F || !Roboto || !Canvas)
	{
		return Roboto;
	}
	// Boyutu Roboto'ya esitle: tasarim olculeri (Scale) Roboto'yla yapildi. Bir kez olculur.
	float& Cached = FontMul[bUseTitleFont && TitleFont ? 1 : 0][bLarge ? 1 : 0];
	if (Cached < 0.f)
	{
		float RW = 0.f, RH = 0.f, FW = 0.f, FH = 0.f;
		Canvas->StrLen(Roboto, TEXT("Ag"), RW, RH);
		Canvas->StrLen(F, TEXT("Ag"), FW, FH);
		Cached = FH > 0.f ? RH / FH : 1.f;
		UE_LOG(LogTemp, Log, TEXT("SL_FONT olcek %s %s: %.2f"), bUseTitleFont ? TEXT("baslik") : TEXT("metin"), bLarge ? TEXT("buyuk") : TEXT("orta"), Cached);
	}
	OutMul = Cached * (bUseTitleFont && TitleFont ? 1.f : BodyFontBoost);
	return F;
}

void ASLHUD::Text(const FString& S, float X, float Y, const FLinearColor& Color, float Scale, bool bLarge)
{
	float Mul = 1.f;
	UFont* Font = PickFont(bLarge, Mul);
	const float Sc = Scale * TextScale * Mul;
	if (bTextShadow && Color.R + Color.G + Color.B > 1.2f)
	{
		// Yalniz koyu test panelinde: acik yaziya golge. Krem kartlarda golge kirli gorunuyor.
		DrawText(S, FLinearColor(0.f, 0.f, 0.f, 0.8f), X + 1.5f, Y + 1.5f, Font, Sc);
	}
	DrawText(S, Color, X, Y, Font, Sc);
}

void ASLHUD::FillRound(float X, float Y, float W, float H, float R, const FLinearColor& C)
{
	// Canvas'ta yuvarlak dikdortgen yok: ust ve alt kavisleri 1 px'lik yatay seritlerle, ortayi tek
	// dikdortgenle ciz. Seritler ust uste binmez, yari saydam renk de duz cikar.
	R = FMath::Clamp(FMath::FloorToFloat(R), 0.f, FMath::FloorToFloat(0.5f * FMath::Min(W, H)));
	for (int32 I = 0; I < int32(R); ++I)
	{
		const float Dy = R - (I + 0.5f);
		const float In = R - FMath::Sqrt(FMath::Max(0.f, R * R - Dy * Dy));
		DrawRect(C, X + In, Y + I, W - 2.f * In, 1.f);
		DrawRect(C, X + In, Y + H - I - 1.f, W - 2.f * In, 1.f);
	}
	DrawRect(C, X, Y + R, W, H - 2.f * R);
}

void ASLHUD::FillCircle(float CX, float CY, float R, const FLinearColor& C)
{
	const int32 Rows = FMath::Max(1, FMath::RoundToInt(2.f * R));
	for (int32 I = 0; I < Rows; ++I)
	{
		const float Dy = (I + 0.5f) - R;
		const float Half = FMath::Sqrt(FMath::Max(0.f, R * R - Dy * Dy));
		DrawRect(C, CX - Half, CY - R + I, 2.f * Half, 1.f);
	}
}

void ASLHUD::FillTri(float CX, float ApexY, float BaseW, float H, const FLinearColor& C)
{
	// Tepesi ApexY'de, tabani ApexY+H'de (H negatifse tepe altta) ikizkenar ucgen.
	const int32 Rows = FMath::Max(1, FMath::RoundToInt(FMath::Abs(H)));
	const float Dir = H >= 0.f ? 1.f : -1.f;
	for (int32 I = 0; I < Rows; ++I)
	{
		const float Wd = BaseW * (I + 0.5f) / Rows;
		DrawRect(C, CX - 0.5f * Wd, Dir > 0.f ? ApexY + I : ApexY - I - 1.f, Wd, 1.f);
	}
}

void ASLHUD::Card(float X, float Y, float W, float H, float R, const FLinearColor& C)
{
	FillRound(X + 1.f, Y + 4.f, W, H, R, FLinearColor(0.30f, 0.20f, 0.40f, 0.16f)); // yumusak golge
	FillRound(X, Y, W, H, R, C);
}

FVector2D ASLHUD::Measure(const FString& S, float Scale, bool bLarge)
{
	float Mul = 1.f;
	UFont* Font = PickFont(bLarge, Mul);
	float W = 0.f, H = 0.f;
	GetTextSize(S, W, H, Font, Scale * TextScale * Mul);
	return FVector2D(W, H);
}

void ASLHUD::Line(float X, float& Y, const FString& S, const FLinearColor& Color)
{
	Text(S, X, Y, Color);
	Y += 22.f * TextScale;
}

TArray<FString> ASLHUD::Wrap(const FString& S, float MaxWidth, float Scale)
{
	TArray<FString> Words, Lines;
	S.ParseIntoArray(Words, TEXT(" "));
	FString Cur;
	for (const FString& W : Words)
	{
		const FString Try = Cur.IsEmpty() ? W : Cur + TEXT(" ") + W;
		if (!Cur.IsEmpty() && Measure(Try, Scale, false).X > MaxWidth)
		{
			Lines.Add(Cur);
			Cur = W;
		}
		else
		{
			Cur = Try;
		}
	}
	if (!Cur.IsEmpty())
	{
		Lines.Add(Cur);
	}
	return Lines;
}

void ASLHUD::SetDrawerOpen(bool bOpen)
{
	if (bOpen != bDrawerWanted)
	{
		ASLGameMode::PlaySfx(this, TEXT("SFX_Cekmece"), 0);
	}
	bDrawerWanted = bOpen;
}

void ASLHUD::Flash(const FString& S, float Seconds)
{
	FlashText = S;
	FlashUntil = GetWorld()->GetTimeSeconds() + Seconds;
}

// --- Cizim ---------------------------------------------------------------------------------------

void ASLHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas)
	{
		return;
	}
	const float Now = GetWorld()->GetRealTimeSeconds();
	const float Dt = LastDrawTime < 0.f ? 0.f : FMath::Min(Now - LastDrawTime, 0.1f);
	LastDrawTime = Now;

	ASLPlayerController* PC = Cast<ASLPlayerController>(GetOwningPlayerController());
	ASLGameMode* GM = GetWorld()->GetAuthGameMode<ASLGameMode>();
	NowReal = Now;
	bAnyHover = false;
	if (PC)
	{
		float MX = -1.f, MY = -1.f;
		MousePos = PC->GetMousePosition(MX, MY) ? FVector2D(MX, MY) : FVector2D(-1000.f, -1000.f);
	}
	const ASLPlatform* P = GM ? GM->GetPlatform() : nullptr;
	const ASLLevelDirector* D = GM ? GM->GetDirector() : nullptr;
	const USLBuildComponent* B = PC ? PC->Build.Get() : nullptr;
	if (!P || !B)
	{
		return;
	}
	MenuRects.Reset();
	MenuActions.Reset();
	if (!Ambience.IsValid())
	{
		// Gece hap zemini (PillAlpha) icin gun isigini okuyoruz; menude DrawSky cizilmedigi icin burada bul.
		for (TActorIterator<ASLAmbience> It(GetWorld()); It; ++It)
		{
			Ambience = *It;
			break;
		}
	}
	if (GM && GM->IsMenu())
	{
		if (GM->bCreditsOnStart && !bCreditsChecked)
		{
			MenuPage = ESLMenuPage::Credits; // son bolum bitince "Emegi gecenler"
			CreditsStart = Now;
		}
		bCreditsChecked = true;
		if (D)
		{
			DrawFamilyOverlays(*GM->GetDirector());
		}
		DrawMenu();
		{
			// Muzik dugmesi menude de.
			const float S = UiScale();
			MusicRect = { Canvas->ClipX - 64.f * S, 28.f * S, 44.f * S, 44.f * S };
			const bool bH = Hover(MusicRect);
			DrawSpeakerIcon(MusicRect.X + 20.f * S, MusicRect.Y + 22.f * S, 22.f * S, GM->IsMusicOn(), bH ? FLinearColor(0.243f, 0.212f, 0.345f) : FLinearColor(0.416f, 0.376f, 0.525f));
		}
		DrawCinemaOverlay();
		if (PC)
		{
			PC->CurrentMouseCursor = bAnyHover ? EMouseCursor::Hand : EMouseCursor::Default;
		}
		return;
	}

	// Gokyuzu pusulasi (16 Eki): eski kartlar (gorev listesi, aileler karti, cekmece, saat karti, denge
	// bari) cizilmiyor; fonksiyonlari duruyor (WBP_HUD'a gecerken basvuru).
	if (D)
	{
		DrawHomeNodes(*D);
		DrawSky(*D);
		DrawConstellation(*GM->GetDirector());
		DrawFamilyOverlays(*GM->GetDirector());
		DrawSpeech(*D);
	}
	DrawCompass(*P, *B);
	DrawRotateButtons();
	if (!bCinema)
	{
		DrawHintC(*P, *B, D);
		FRect R;
		MenuButton(TEXT("Menü"), 48.f * UiScale(), 34.f * UiScale(), 0.72f * UiScale(), R);
		MenuRects.Add(R);
		MenuActions.Add(21);
	}
	if (!bCinema)
	{
		DrawEndC(*P, GM->IsLevelComplete()); // trailerda bitis yazisi ve dugmeleri son 15 sn ekranda kaliyordu
	}
	DrawCinemaOverlay();
	if (PC)
	{
		PC->CurrentMouseCursor = bAnyHover ? EMouseCursor::Hand : EMouseCursor::Default;
	}
	if (bShowTestPanel)
	{
		DrawTestPanel(*P, *B);
	}
}

float ASLHUD::PredictPlacement(const ASLPlatform& P, const USLBuildComponent& B) const
{
	const ASLPlaceable* G = B.GetGhost();
	if (!G || B.State != ESLBuildState::Placing)
	{
		return -1.f;
	}
	return P.PredictTiltDegrees(G->Mass, B.GetGhostCenterLocal());
}

void ASLHUD::DrawBalanceBar(const ASLPlatform& P, const USLBuildComponent& B)
{
	// 0..FailTilt*1.2 olcegi: turkuaz (dengede), sari (uyari), mercan (devrilme). Sallanma haric egim.
	const float W = BalanceBarWidth, H = 16.f;
	const FString Label = TEXT("Denge");
	const FVector2D LSz = Measure(Label, 0.85f, false);
	const float CardW = W + LSz.X + 70.f, BarCardH = 46.f;
	const float CX = 0.5f * (Canvas->ClipX - CardW), CY = 16.f;
	const float X = CX + LSz.X + 50.f, Y = CY + 0.5f * (BarCardH - H);
	const float Max = FMath::Max(P.FailTilt * 1.2f, 0.1f);
	const float Judged = P.IsCollapsed() ? Max : P.CurrentTilt.Size();
	const float WarnX = W * FMath::Clamp(P.WarnTilt / Max, 0.f, 1.f);
	const float FailX = W * FMath::Clamp(P.FailTilt / Max, 0.f, 1.f);

	Card(CX, CY, CardW, BarCardH, 0.5f * BarCardH, PanelColor);
	// Terazi simgesi: kucuk ucgen ayak + kol + iki kefe.
	const float IX = CX + 22.f, IY = CY + 0.5f * BarCardH;
	FillTri(IX, IY - 2.f, 12.f, 10.f, Ink);
	FillRound(IX - 10.f, IY - 6.f, 20.f, 3.f, 1.5f, Ink);
	FillCircle(IX - 9.f, IY - 1.f, 3.f, Teal);
	FillCircle(IX + 9.f, IY - 1.f, 3.f, Teal);
	Text(Label, CX + 38.f, CY + 0.5f * (BarCardH - LSz.Y), Ink, 0.85f);

	// Iz: uc bolge soluk renkte, hap biciminde.
	FillRound(X, Y, W, H, 0.5f * H, FLinearColor(0.90f, 0.88f, 0.86f));
	FillRound(X, Y, WarnX, H, 0.5f * H, Pale(Teal));
	DrawRect(Pale(Yellow), X + WarnX, Y, FailX - WarnX, H);
	FillRound(X + FailX - 0.5f * H, Y, W - FailX + 0.5f * H, H, 0.5f * H, Pale(Red));
	DrawRect(Pale(Yellow), X + WarnX, Y, FailX - WarnX, H); // mercanin yuvarlak basini sariyla ort

	const FLinearColor Fill = Judged >= P.FailTilt ? Red : Judged >= P.WarnTilt ? Yellow : Teal;
	const float FillW = W * FMath::Clamp(Judged / Max, 0.f, 1.f);
	if (FillW > 1.f)
	{
		FillRound(X, Y + 2.f, FMath::Max(FillW, H - 4.f), H - 4.f, 0.5f * (H - 4.f), Fill);
	}
	// Esik isaretleri: izin ustunde ince beyaz ayraclar.
	DrawRect(FLinearColor::White, X + WarnX - 1.f, Y - 3.f, 2.f, H + 6.f);
	DrawRect(FLinearColor::White, X + FailX - 1.f, Y - 3.f, 2.f, H + 6.f);

	// Tahmin: elindekini buraya koyarsan bar nereye gelir. Ince cizgi + ustunde yuvarlak bas.
	const float Pred = PredictPlacement(P, B);
	if (Pred >= 0.f)
	{
		const float PX = X + W * FMath::Clamp(Pred / Max, 0.f, 1.f);
		const FLinearColor PCol = Pred >= P.FailTilt ? Red : Pred >= P.WarnTilt ? Yellow : Ink;
		DrawRect(PCol, PX - 1.5f, Y - 5.f, 3.f, H + 10.f);
		FillCircle(PX, Y - 8.f, 5.f, PCol);
	}

	if (P.BalanceState == ESLBalanceState::Failing)
	{
		// Devrilme sayaci: kartin altinda azalan serit.
		const float Left = FMath::Clamp(1.f - P.FailTimer / FMath::Max(P.FailSeconds, 0.01f), 0.f, 1.f);
		FillRound(X, CY + BarCardH + 6.f, FMath::Max(W * Left, 6.f), 6.f, 3.f, Red);
	}
}

void ASLHUD::BuildHint(const ASLPlatform& P, const USLBuildComponent& B, const ASLLevelDirector* D, FString& Out, FLinearColor& Color) const
{
	Color = Info;
	if (P.IsCollapsed() || (D && D->bLevelComplete))
	{
		return; // ortadaki mesaj yeterli
	}
	if (GetWorld()->GetTimeSeconds() < FlashUntil && !FlashText.IsEmpty())
	{
		Out = FlashText;
		Color = Yellow;
		return;
	}
	if (P.BalanceState == ESLBalanceState::Failing)
	{
		Out = TEXT("Platform devriliyor! Ağır bir binaya tıklayıp kaldır ya da karşı tarafa ağırlık koy.");
		Color = Red;
		return;
	}

	const ASLPlaceable* G = B.GetGhost();
	if (G && B.State != ESLBuildState::Idle)
	{
		if (B.State == ESLBuildState::Blocked)
		{
			Out = FString::Printf(TEXT("Buraya konmaz: %s."), B.GetBlockedReason());
			Color = Red;
			return;
		}
		const float Pred = PredictPlacement(P, B);
		if (Pred >= P.FailTilt)
		{
			Out = TEXT("Buraya koyarsan platform DEVRİLİR. Merkeze yaklaştır ya da karşı tarafa koy.");
			Color = Red;
			return;
		}
		if (Pred >= P.WarnTilt)
		{
			Out = TEXT("Buraya koyarsan denge sınıra gelir. Biraz daha merkeze yaklaştır.");
			Color = Yellow;
			return;
		}
		if (G->bHasEntrance)
		{
			// Kapi onu: hepsi disarida ya da hepsi baska binayla dolu mu?
			const int32 N = P.GetCellCount();
			int32 NumOut = 0, NumFull = 0;
			const TArray<FIntPoint> Cells = G->GetEntranceCells();
			for (const FIntPoint& E : Cells)
			{
				if (E.X < 0 || E.Y < 0 || E.X >= N || E.Y >= N)
				{
					++NumOut;
				}
				else if (const ASLPlaceable* At = P.FindPlaceableAt(E); At && At->TypeId != TEXT("Yol"))
				{
					++NumFull;
				}
			}
			if (Cells.Num() > 0 && NumOut == Cells.Num())
			{
				Out = TEXT("Kapı platformun dışına bakıyor: yol bağlanamaz. R / Q ile çevir.");
				Color = Yellow;
				return;
			}
			if (Cells.Num() > 0 && NumOut + NumFull == Cells.Num())
			{
				Out = TEXT("Kapının önü başka bir binayla kapalı. R / Q ile çevir ya da yer değiştir.");
				Color = Yellow;
				return;
			}
		}
		{
			FString Home;
			bool bGood = false;
			if (D && D->DescribeHomePlacement(G, B.GetMoving(), Home, bGood))
			{
				Out = Home;
				Color = bGood ? Teal : Yellow;
				return;
			}
		}
		if (B.IsMoving())
		{
			Out = TEXT("Elindeki binanın ağırlığı şu an platformda değil. Yeni yerini seç; sağ tık eski yerine koyar.");
			return;
		}
		if (G->TypeId == TEXT("Yol"))
		{
			Out = TEXT("Yolu binaların kapısının önündeki sarı karelere ulaştır.");
			return;
		}
		Out = TEXT("Yeşil: buraya konabilir. Bardaki beyaz çizgi koyduktan sonraki eğim. Sarı kare kapının önü.");
		return;
	}

	if (P.BalanceState == ESLBalanceState::Warning)
	{
		const ASLCameraPawn* Cam = Cast<ASLCameraPawn>(GetOwningPawn());
		Out = FString::Printf(TEXT("Platform %s yatıyor. Sıradaki binayı karşı tarafa koy ya da bir binayı tıklayıp taşı."),
			*P.DescribeLeanSide(Cam ? Cam->Yaw : 45.f));
		Color = Yellow;
		return;
	}

	if (D && D->bOpenLevel)
	{
		// Acik bolum: oyuncuya sira degil durum soylenir.
		bool bUnknown = false, bHomeless = false;
		for (int32 I = 0; I < D->Families.Num(); ++I)
		{
			bUnknown |= D->IsFamilyPresent(I) && !D->IsFamilyDiscovered(I);
			bHomeless |= D->IsFamilyPresent(I) && D->IsFamilyDiscovered(I) && !D->GetHome(I);
		}
		const int32 Sel = D->GetSelectedFamily();
		if (Sel != INDEX_NONE && !D->GetHome(Sel))
		{
			Out = FString::Printf(TEXT("%s seçili: pusuladan Ev'i seçip koy ya da boş bir eve tıkla."), *D->Families[Sel].Name);
		}
		else if (bUnknown)
		{
			Out = TEXT("Soru işaretli ailelere tıkla: ne istediklerini öğren.");
		}
		else if (bHomeless)
		{
			Out = TEXT("Bir aileye tıkla, sonra ona ev kur ya da boş bir eve tıkla.");
		}
		else
		{
			Out = D->DescribeOpenGoal() + TEXT(". Z / X tahtayı çevirir.");
		}
		return;
	}
	if (D && D->Tasks.IsValidIndex(D->CurrentTask))
	{
		const FSLTask& T = D->Tasks[D->CurrentTask];
		switch (T.Kind)
		{
		case ESLTaskKind::PlaceType:
		{
			const int32 Slot = B.FindSlotById(T.TypeId);
			const FString Label = B.Slots.IsValidIndex(Slot) ? B.Slots[Slot].Label : T.TypeId.ToString();
			Out = IsDrawerOpen()
				? FString::Printf(TEXT("Sol alttaki pusuladan %s seç."), *Label)
				: FString::Printf(TEXT("Sol alttaki pusuladan %s seç (kısayol: %d)."), *Label, Slot + 1);
			return;
		}
		case ESLTaskKind::FamilyMovedIn:
			Out = TEXT("Aile kapıya yürüyor. Aile ağırlık yapmaz, platformu izle.");
			return;
		case ESLTaskKind::ConnectRoads:
			Out = FString::Printf(TEXT("Pusuladan Yol seç. Bağlı bina: %d / %d. Kapısı yanlış yöne bakan binayı tıklayıp R / Q ile çevir."),
				D->Progress, D->ProgressGoal);
			return;
		case ESLTaskKind::FamiliesHappy:
			Out = FString::Printf(TEXT("Mutlu aile: %d / %d. %s"), D->Progress, D->ProgressGoal, *T.Hint);
			return;
		}
	}
}

void ASLHUD::DrawHint(const ASLPlatform& P, const USLBuildComponent& B, const ASLLevelDirector* D)
{
	FString S;
	FLinearColor C;
	BuildHint(P, B, D, S, C);
	LastHint = S;
	if (S.IsEmpty())
	{
		return;
	}
	const float Scale = 0.8f;
	const TArray<FString> Lines = Wrap(S, 600.f, Scale);
	const float LineH = Measure(TEXT("Ag"), Scale, false).Y + 2.f;
	float W = 0.f;
	for (const FString& L : Lines)
	{
		W = FMath::Max(W, Measure(L, Scale, false).X);
	}
	const float X = 0.5f * (Canvas->ClipX - W) + 14.f, Y = 84.f;
	const float BH = LineH * Lines.Num() + 18.f;
	Card(X - 44.f, Y - 9.f, W + 62.f, BH, 14.f, PanelColor);
	// Renkli rozet: turkuaz bilgi / sari uyari / mercan tehlike; icinde "i" ya da "!".
	FillCircle(X - 22.f, Y - 9.f + 0.5f * BH, 10.f, C);
	const FString Glyph = (C.Equals(Teal) || C.Equals(Info)) ? TEXT("i") : TEXT("!");
	const FVector2D GSz = Measure(Glyph, 0.75f, false);
	Text(Glyph, X - 22.f - 0.5f * GSz.X, Y - 9.f + 0.5f * (BH - GSz.Y), FLinearColor::White, 0.75f);
	for (int32 I = 0; I < Lines.Num(); ++I)
	{
		Text(Lines[I], X, Y + I * LineH, Ink, Scale);
	}
}

void ASLHUD::DrawSpeech(const ASLLevelDirector& D)
{
	FString S;
	FVector Anchor;
	if (!D.GetSpeech(S, Anchor))
	{
		return;
	}
	const FVector Screen = Project(Anchor);
	if (Screen.Z <= 0.f)
	{
		return; // kameranin arkasinda
	}
	const float Scale = 0.85f;
	const TArray<FString> Lines = Wrap(S, 330.f, Scale);
	const float LineH = Measure(TEXT("Ag"), Scale, false).Y + 2.f;
	float W = 0.f;
	for (const FString& L : Lines)
	{
		W = FMath::Max(W, Measure(L, Scale, false).X);
	}
	const float Pad = 14.f, TailH = 16.f;
	const float BW = W + 2.f * Pad, BH = LineH * Lines.Num() + 2.f * Pad;
	// Balon capanin ustunde; ekrandan tasmasin.
	const float BX = FMath::Clamp(Screen.X - 0.5f * BW, 12.f, Canvas->ClipX - BW - 12.f);
	const float BY = FMath::Clamp(Screen.Y - BH - TailH, 140.f, Canvas->ClipY - BH - 12.f);
	OutlineRound(BX, BY, BW, BH, 12.f, 1.f, FLinearColor(0.62f, 0.573f, 0.745f), FLinearColor(0.984f, 0.973f, 0.992f, 0.98f));
	// Kuyruk: capaya dogru daralan ucgen.
	const float TX = FMath::Clamp(Screen.X, BX + 22.f, BX + BW - 22.f);
	for (int32 I = 0; I < int32(TailH); ++I)
	{
		const float Wd = 20.f * (1.f - float(I) / TailH);
		DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.98f), TX - 0.5f * Wd, BY + BH + I, Wd, 1.f);
	}
	for (int32 I = 0; I < Lines.Num(); ++I)
	{
		Text(Lines[I], BX + Pad, BY + Pad + I * LineH, Ink, Scale);
	}
}

void ASLHUD::DrawTasks(const ASLLevelDirector& D)
{
	if (D.bOpenLevel)
	{
		// Acik bolum: sirali liste yok. Hedef + mahalle plani (kurulan turuncu degil turkuaz).
		const ASLPlayerController* PC = Cast<ASLPlayerController>(GetOwningPlayerController());
		const USLBuildComponent* B = PC ? PC->Build.Get() : nullptr;
		const float X = 20.f, Y0 = 16.f, W = 300.f;
		const float LineH = 26.f * TextScale;
		const float H = 18.f + LineH * (2.6f + D.RequiredTypes.Num()) + 6.f;
		Card(X, Y0, W, H, 18.f, PanelColor);
		const float PinX = X + 26.f, PinY = Y0 + 26.f;
		FillTri(PinX, PinY + 15.f, 13.f, -12.f, Red);
		FillCircle(PinX, PinY, 8.f, Red);
		FillCircle(PinX, PinY, 3.f, FLinearColor::White);
		Text(D.LevelTitle, X + 44.f, PinY - 0.5f * Measure(D.LevelTitle, 1.f, false).Y, Ink, 1.f);
		float Y = Y0 + 18.f + LineH * 1.3f;
		Text(TEXT("Herkes mutlu olsun"), X + 44.f, Y, Ink, 0.8f);
		Y += LineH;
		Text(TEXT("Mahalle planı"), X + 44.f, Y, Muted, 0.65f);
		Y += 0.8f * LineH;
		for (const FName& T : D.RequiredTypes)
		{
			const bool bOk = D.IsRequiredPlaced(T);
			const int32 S = B ? B->FindSlotById(T) : INDEX_NONE;
			const FString Label = (B && B->Slots.IsValidIndex(S)) ? B->Slots[S].Label : T.ToString();
			FillCircle(X + 50.f, Y + 9.f, 6.f, bOk ? Teal : FLinearColor(0.80f, 0.78f, 0.80f));
			Text(Label, X + 64.f, Y, bOk ? Muted : Ink, 0.72f);
			Y += LineH * 0.85f;
		}
		TasksBottom = Y0 + H;
		return;
	}
	// Sol ust kart: konum rozeti + bolum adi, altinda yuvarlak onay kutulu gorevler. Sag taraf cekmecenin.
	const float TitleScale = 1.0f, ItemScale = 0.8f;
	float W = Measure(D.LevelTitle, TitleScale, false).X + 40.f;
	for (const FSLTask& T : D.Tasks)
	{
		W = FMath::Max(W, Measure(T.Text + TEXT("  0/0"), ItemScale, false).X + 34.f);
	}
	const float LineH = 26.f * TextScale;
	const float X = 20.f, Y0 = 16.f;
	// Uzun bolumde (3. bolum 12 gorev) liste kartlari ekrandan tasiriyordu: bitenlerden yalnizca son 1'i,
	// ustte "n görev tamam" satiri; ileriden en fazla 4 gorev.
	constexpr int32 MaxShown = 7;
	int32 First = 0, Last = D.Tasks.Num() - 1;
	if (D.Tasks.Num() > MaxShown)
	{
		First = FMath::Clamp(D.CurrentTask - 1, 0, D.Tasks.Num() - MaxShown);
		Last = First + MaxShown - 1;
	}
	const int32 Rows = Last - First + 1 + (First > 0 ? 1 : 0);
	const float H = LineH * (Rows + 1.5f) + 18.f;
	Card(X, Y0, W + 36.f, H, 18.f, PanelColor);
	TasksBottom = Y0 + H;

	// Konum rozeti: mercan daire + beyaz ic nokta + asagi ucgen.
	const float PinX = X + 26.f, PinY = Y0 + 26.f;
	FillTri(PinX, PinY + 15.f, 13.f, -12.f, Red); // ters ucgen: tepesi asagida
	FillCircle(PinX, PinY, 8.f, Red);
	FillCircle(PinX, PinY, 3.f, FLinearColor::White);
	const FVector2D TSz = Measure(D.LevelTitle, TitleScale, false);
	Text(D.LevelTitle, X + 44.f, PinY - 0.5f * TSz.Y, Ink, TitleScale);

	float Y = Y0 + 18.f + LineH * 1.3f;
	if (First > 0)
	{
		Text(FString::Printf(TEXT("%d görev tamam"), First), X + 44.f, Y + 0.5f * LineH - 2.f - 0.5f * Measure(TEXT("A"), ItemScale, false).Y, Teal, ItemScale);
		Y += LineH;
	}
	for (int32 I = First; I <= Last; ++I)
	{
		const FSLTask& T = D.Tasks[I];
		const bool bDone = D.IsTaskDone(I);
		const bool bCurrent = I == D.CurrentTask && !D.bLevelComplete;
		const float CX = X + 26.f, CY = Y + 0.5f * LineH - 2.f;
		if (bDone)
		{
			FillCircle(CX, CY, 9.f, Teal);
			DrawLine(CX - 4.5f, CY, CX - 1.f, CY + 3.5f, FLinearColor::White, 2.5f);
			DrawLine(CX - 1.f, CY + 3.5f, CX + 5.f, CY - 3.5f, FLinearColor::White, 2.5f);
		}
		else
		{
			FillCircle(CX, CY, 9.f, bCurrent ? Teal : FLinearColor(0.75f, 0.72f, 0.74f));
			FillCircle(CX, CY, 6.5f, PanelColor);
		}
		FString S = T.Text;
		if (bCurrent && D.ProgressGoal > 1)
		{
			S += FString::Printf(TEXT("  %d/%d"), D.Progress, D.ProgressGoal);
		}
		const FVector2D Sz = Measure(S, ItemScale, false);
		Text(S, X + 44.f, CY - 0.5f * Sz.Y, bDone ? Muted : bCurrent ? Ink : FLinearColor(Muted.R, Muted.G, Muted.B, 0.7f), ItemScale);
		Y += LineH;
	}
}

void ASLHUD::DrawFace(float CX, float CY, float R, float Happy01)
{
	// Mutlu (turkuaz, gulen) / idare eder (sari, duz) / mutsuz (mercan, asik). Emoji yok: sekiller.
	const bool bHappy = Happy01 >= 0.999f, bMid = !bHappy && Happy01 >= 0.5f;
	const FLinearColor C = bHappy ? Teal : bMid ? Yellow : Red;
	FillCircle(CX, CY, R, C);
	const FLinearColor Eye = FLinearColor::White;
	FillCircle(CX - 0.35f * R, CY - 0.2f * R, FMath::Max(1.5f, 0.13f * R), Eye);
	FillCircle(CX + 0.35f * R, CY - 0.2f * R, FMath::Max(1.5f, 0.13f * R), Eye);
	const float T = FMath::Max(1.5f, 0.14f * R);
	const float MY = CY + 0.32f * R, W = 0.42f * R, Bend = 0.18f * R;
	if (bMid)
	{
		DrawLine(CX - W, MY, CX + W, MY, Eye, T);
	}
	else
	{
		const float S = bHappy ? 1.f : -1.f; // gulumseme asagi kavis, asik yukari
		const float Y0 = MY - S * 0.5f * Bend;
		DrawLine(CX - W, Y0, CX - 0.4f * W, Y0 + S * Bend, Eye, T);
		DrawLine(CX - 0.4f * W, Y0 + S * Bend, CX + 0.4f * W, Y0 + S * Bend, Eye, T);
		DrawLine(CX + 0.4f * W, Y0 + S * Bend, CX + W, Y0, Eye, T);
	}
}

void ASLHUD::DrawSpeakerIcon(float CX, float CY, float S, bool bOn, const FLinearColor& C)
{
	DrawRect(C, CX - 0.45f * S, CY - 0.18f * S, 0.25f * S, 0.36f * S);
	for (int32 I = 0; I < int32(0.35f * S); ++I)
	{
		const float Hh = 0.18f * S + I * 0.9f;
		DrawRect(C, CX - 0.2f * S + I, CY - Hh, 1.f, 2.f * Hh);
	}
	if (bOn)
	{
		DrawLine(CX + 0.25f * S, CY - 0.2f * S, CX + 0.3f * S, CY, C, 2.f);
		DrawLine(CX + 0.3f * S, CY, CX + 0.25f * S, CY + 0.2f * S, C, 2.f);
		DrawLine(CX + 0.4f * S, CY - 0.35f * S, CX + 0.48f * S, CY, C, 2.f);
		DrawLine(CX + 0.48f * S, CY, CX + 0.4f * S, CY + 0.35f * S, C, 2.f);
	}
	else
	{
		DrawLine(CX + 0.22f * S, CY - 0.25f * S, CX + 0.5f * S, CY + 0.25f * S, Red, 2.5f);
		DrawLine(CX + 0.5f * S, CY - 0.25f * S, CX + 0.22f * S, CY + 0.25f * S, Red, 2.5f);
	}
}

void ASLHUD::DrawClock(const ASLLevelDirector& D)
{
	if (!Ambience.IsValid())
	{
		for (TActorIterator<ASLAmbience> It(GetWorld()); It; ++It)
		{
			Ambience = *It;
			break;
		}
	}
	const float Abs = D.GetTimeHours();
	const float Hour = Ambience.IsValid() ? Ambience->GetShownHour() : FMath::Fmod(Abs, 24.f);
	const float Day01 = Ambience.IsValid() ? Ambience->GetDaylight() : ASLAmbience::DaylightAt(Hour);
	const int32 DayN = FMath::FloorToInt(Abs / 24.f) + 1;
	const int32 Left = D.GetMovesLeft();
	const float W = 230.f, H = Left >= 0 ? 74.f : 50.f;
	const float X = Canvas->ClipX - W - 16.f, Y = 16.f;
	Card(X, Y, W, H, 20.f, PanelColor);
	// Gunes (sari daire + isinlar) ya da ay (hilal: daire + kart renginde kaydirilmis daire).
	const float IX = X + 28.f, IY = Y + 25.f;
	if (Day01 >= 0.5f)
	{
		for (int32 I = 0; I < 8; ++I)
		{
			const float A = I * UE_PI / 4.f + NowReal * 0.3f;
			DrawLine(IX + 10.f * FMath::Cos(A), IY + 10.f * FMath::Sin(A), IX + 14.f * FMath::Cos(A), IY + 14.f * FMath::Sin(A), Yellow, 2.f);
		}
		FillCircle(IX, IY, 8.f, Yellow);
	}
	else
	{
		FillCircle(IX, IY, 10.f, FLinearColor(0.55f, 0.60f, 0.95f));
		FillCircle(IX + 5.f, IY - 3.f, 9.f, PanelColor);
	}
	const FString Clock = FString::Printf(TEXT("%d. gün  %02d:%02d"), DayN, FMath::FloorToInt(Hour), FMath::FloorToInt(FMath::Frac(Hour) * 60.f));
	const FVector2D CS = Measure(Clock, 0.85f, false);
	Text(Clock, X + 50.f, IY - 0.5f * CS.Y, Ink, 0.85f);
	if (Left >= 0)
	{
		const FString M = FString::Printf(TEXT("Kalan hamle: %d"), Left);
		Text(M, X + 50.f, Y + 46.f, Left <= 3 ? Red : Muted, 0.7f);
	}
	// Muzik dugmesi: hoparlor, ustune gelince daire.
	MusicRect = { X + W - 44.f, Y + 7.f, 36.f, 36.f };
	const bool bH = Hover(MusicRect);
	const ASLGameMode* GM = GetWorld()->GetAuthGameMode<ASLGameMode>();
	if (bH)
	{
		FillCircle(MusicRect.X + 18.f, MusicRect.Y + 18.f, 17.f, Pale(Teal));
	}
	DrawSpeakerIcon(MusicRect.X + 16.f, MusicRect.Y + 18.f, 22.f, GM && GM->IsMusicOn(), bH ? Teal : Muted);
}

void ASLHUD::DrawFamilies(const ASLLevelDirector& D)
{
	if (!D.HasFamilyNeeds())
	{
		return;
	}
	const float X = 20.f, Y0 = TasksBottom + 12.f;
	const float W = 300.f, RowH = 64.f;
	const float H = 46.f + RowH * D.Families.Num();
	Card(X, Y0, W, H, 18.f, PanelColor);
	Text(TEXT("Aileler"), X + 20.f, Y0 + 12.f, Ink, 0.9f);
	float Y = Y0 + 44.f;
	for (int32 I = 0; I < D.Families.Num(); ++I, Y += RowH)
	{
		const bool bPresent = D.IsFamilyPresent(I);
		const float Hp = D.GetDisplayHappiness(I);
		if (bPresent)
		{
			DrawFace(X + 34.f, Y + 18.f, 14.f, D.GetHappiness(I));
		}
		else
		{
			FillCircle(X + 34.f, Y + 18.f, 14.f, FLinearColor(0.85f, 0.83f, 0.84f));
		}
		Text(D.Families[I].Name, X + 58.f, Y + 2.f, bPresent ? Ink : Muted, 0.75f);
		const FString Pct = bPresent ? FString::Printf(TEXT("%d%%"), FMath::RoundToInt(100.f * Hp)) : FString(TEXT("-"));
		const FVector2D PSz = Measure(Pct, 0.7f, false);
		Text(Pct, X + W - 18.f - PSz.X, Y + 2.f, Muted, 0.7f);
		// Bar: soluk iz + durum renginde dolgu.
		const float BX = X + 58.f, BW = W - 76.f, BY = Y + 24.f;
		FillRound(BX, BY, BW, 8.f, 4.f, FLinearColor(0.90f, 0.88f, 0.86f));
		if (bPresent && Hp > 0.01f)
		{
			const FLinearColor C = Hp >= 0.999f ? Teal : Hp >= 0.5f ? Yellow : Red;
			FillRound(BX, BY, FMath::Max(8.f, BW * Hp), 8.f, 4.f, C);
		}
		Text(D.GetFamilyStatus(I), BX, Y + 36.f, Muted, 0.6f);
	}
}

void ASLHUD::DrawHomeFaces(const ASLLevelDirector& D)
{
	if (!D.HasFamilyNeeds())
	{
		return;
	}
	for (int32 I = 0; I < D.Families.Num(); ++I)
	{
		const ASLPlaceable* Home = D.GetHome(I);
		if (!Home || Home->bHeld || !Home->bResidentsArrived)
		{
			continue;
		}
		const FBox Bx = Home->GetComponentsBoundingBox(false);
		const FVector S = Project(FVector(Bx.Max.X, Bx.GetCenter().Y, Bx.Max.Z + 6.f));
		if (S.Z > 0.f)
		{
			FillCircle(S.X + 1.f, S.Y + 3.f, 14.f, FLinearColor(0.30f, 0.20f, 0.40f, 0.18f)); // golge
			FillCircle(S.X, S.Y, 14.f, FLinearColor::White);
			DrawFace(S.X, S.Y, 12.f, D.GetHappiness(I));
		}
	}
}

void ASLHUD::DrawHouseIcon(float CX, float CY, float S, const FLinearColor& C)
{
	FillTri(CX, CY - 0.55f * S, 1.1f * S, 0.5f * S, C);
	DrawRect(C, CX - 0.38f * S, CY - 0.06f * S, 0.76f * S, 0.55f * S);
	DrawRect(PanelColor, CX - 0.1f * S, CY + 0.2f * S, 0.2f * S, 0.3f * S); // kapi
}

void ASLHUD::DrawLockIcon(float CX, float CY, float S, const FLinearColor& C)
{
	FillCircle(CX, CY - 0.18f * S, 0.32f * S, C);
	FillCircle(CX, CY - 0.18f * S, 0.19f * S, FLinearColor(0.95f, 0.94f, 0.93f));
	FillRound(CX - 0.42f * S, CY - 0.12f * S, 0.84f * S, 0.62f * S, 0.12f * S, C);
}

void ASLHUD::DrawSlotThumb(const FSLSlot& S, float X, float Y, float Size, bool bOpen)
{
	// Kucuk sahne: pastel karo zemin, ustunde binanin sembolik cizimi. Mert'in ikonlari gelince yerini alir.
	FillRound(X, Y, Size, Size, 12.f, bOpen ? FLinearColor(0.80f, 0.92f, 0.84f) : FLinearColor(0.88f, 0.87f, 0.87f));
	const float CX = X + 0.5f * Size;
	const float Base = Y + 0.8f * Size;
	const FLinearColor Body = bOpen ? FLinearColor(0.99f, 0.95f, 0.88f) : FLinearColor(0.80f, 0.79f, 0.79f);
	const FLinearColor Accent = bOpen ? Red : FLinearColor(0.65f, 0.64f, 0.64f);
	const FLinearColor Glass = bOpen ? FLinearColor(0.45f, 0.62f, 0.88f) : FLinearColor(0.70f, 0.70f, 0.72f);
	if (S.bFlat)
	{
		// Yol: gri serit, ortasinda kesik cizgi.
		FillRound(X + 0.1f * Size, Y + 0.4f * Size, 0.8f * Size, 0.22f * Size, 0.11f * Size, bOpen ? FLinearColor(0.62f, 0.62f, 0.66f) : FLinearColor(0.75f, 0.75f, 0.76f));
		for (int32 I = 0; I < 4; ++I)
		{
			DrawRect(FLinearColor::White, X + (0.17f + 0.18f * I) * Size, Y + 0.5f * Size, 0.09f * Size, 2.f);
		}
		return;
	}
	if (S.Id == TEXT("Durak"))
	{
		// Sundurma: iki direk, turkuaz cati, bank, yanda yuvarlak tabela.
		const FLinearColor Post = bOpen ? FLinearColor(0.55f, 0.55f, 0.60f) : FLinearColor(0.72f, 0.72f, 0.73f);
		DrawRect(Post, CX - 0.3f * Size, Base - 0.42f * Size, 0.04f * Size, 0.42f * Size);
		DrawRect(Post, CX + 0.1f * Size, Base - 0.42f * Size, 0.04f * Size, 0.42f * Size);
		FillRound(CX - 0.36f * Size, Base - 0.48f * Size, 0.56f * Size, 0.08f * Size, 3.f, bOpen ? Teal : Accent);
		DrawRect(Glass, CX - 0.26f * Size, Base - 0.38f * Size, 0.36f * Size, 0.2f * Size);
		DrawRect(FLinearColor(0.80f, 0.55f, 0.40f), CX - 0.26f * Size, Base - 0.14f * Size, 0.36f * Size, 0.05f * Size);
		DrawRect(Post, CX + 0.3f * Size, Base - 0.5f * Size, 0.03f * Size, 0.5f * Size);
		FillCircle(CX + 0.315f * Size, Base - 0.52f * Size, 0.09f * Size, Accent);
		const FVector2D Sz = Measure(TEXT("D"), 0.45f, false);
		Text(TEXT("D"), CX + 0.315f * Size - 0.5f * Sz.X, Base - 0.52f * Size - 0.5f * Sz.Y, FLinearColor::White, 0.45f);
	}
	else if (S.Id == TEXT("Ev"))
	{
		const float BW = 0.42f * Size, BH = 0.3f * Size;
		DrawRect(Body, CX - 0.5f * BW, Base - BH, BW, BH);
		FillTri(CX, Base - BH - 0.24f * Size, BW * 1.3f, 0.24f * Size, Accent);
		DrawRect(Accent * 0.85f, CX + 0.12f * Size, Base - BH - 0.24f * Size, 0.07f * Size, 0.14f * Size); // baca
		DrawRect(FLinearColor(0.55f, 0.33f, 0.24f), CX - 0.05f * Size, Base - 0.16f * Size, 0.1f * Size, 0.16f * Size);
		DrawRect(Glass, CX - 0.17f * Size, Base - 0.22f * Size, 0.08f * Size, 0.08f * Size);
		DrawRect(Glass, CX + 0.09f * Size, Base - 0.22f * Size, 0.08f * Size, 0.08f * Size);
	}
	else if (S.Id == TEXT("Hastane"))
	{
		const float BW = 0.5f * Size, BH = 0.56f * Size;
		DrawRect(Body, CX - 0.5f * BW, Base - BH, BW, BH);
		DrawRect(Accent, CX - 0.1f * Size, Base - BH + 0.06f * Size, 0.2f * Size, 0.06f * Size);
		DrawRect(Accent, CX - 0.03f * Size, Base - BH - 0.01f * Size, 0.06f * Size, 0.2f * Size);
		for (int32 R = 0; R < 2; ++R)
		{
			for (int32 C = 0; C < 3; ++C)
			{
				DrawRect(Glass, CX - 0.19f * Size + C * 0.14f * Size, Base - 0.3f * Size + R * 0.12f * Size, 0.09f * Size, 0.07f * Size);
			}
		}
	}
	else
	{
		const float BW = 0.62f * Size, BH = 0.5f * Size;
		DrawRect(Body, CX - 0.5f * BW, Base - BH, BW, BH);
		DrawRect(Glass, CX - 0.27f * Size, Base - 0.18f * Size, 0.54f * Size, 0.16f * Size);
		DrawRect(bOpen ? Teal : Accent, CX - 0.3f * Size, Base - 0.23f * Size, 0.6f * Size, 0.05f * Size);
		FillRound(CX - 0.2f * Size, Base - BH + 0.05f * Size, 0.4f * Size, 0.13f * Size, 3.f, Accent);
		const FVector2D Sz = Measure(TEXT("AVM"), 0.45f, false);
		Text(TEXT("AVM"), CX - 0.5f * Sz.X, Base - BH + 0.05f * Size + 0.5f * (0.13f * Size - Sz.Y), FLinearColor::White, 0.45f);
	}
	DrawRect(Pale(Teal), X + 0.08f * Size, Base, 0.84f * Size, 2.f); // zemin cizgisi
}

void ASLHUD::DrawDrawer(const USLBuildComponent& B, float DeltaSeconds)
{
	const ASLPlayerController* PC = Cast<ASLPlayerController>(GetOwningPlayerController());
	if (bDragging && PC)
	{
		float MX = 0.f, MY = 0.f;
		PC->GetMousePosition(MX, MY);
		bDragMoved |= FMath::Abs(MX - DragStartMouseX) > 4.f;
		DrawerOpen = FMath::Clamp(DragStartOpen + (DragStartMouseX - MX) / DrawerWidth, 0.f, 1.f);
	}
	else
	{
		DrawerOpen = FMath::FInterpConstantTo(DrawerOpen, bDrawerWanted ? 1.f : 0.f, DeltaSeconds, DrawerSpeed);
	}

	// Bolumun hic acmadigi slotlar (1. bolumde Durak) cekmecede gorunmez.
	CardSlots.Reset();
	{
		const ASLGameMode* GM = GetWorld()->GetAuthGameMode<ASLGameMode>();
		const ASLLevelDirector* Dir = GM ? GM->GetDirector() : nullptr;
		for (int32 I = 0; I < B.Slots.Num(); ++I)
		{
			bool bUsed = !Dir;
			for (int32 T = 0; Dir && T < Dir->Tasks.Num() && !bUsed; ++T)
			{
				bUsed = Dir->Tasks[T].Unlocks.Contains(B.Slots[I].Id);
			}
			if (bUsed)
			{
				CardSlots.Add(I);
			}
		}
	}
	const int32 N = CardSlots.Num();
	const float HeaderH = 58.f;
	const float PanelH = HeaderH + N * (CardH + 10.f) + 6.f;
	const float Margin = 16.f;
	const float PX = Canvas->ClipX - (DrawerWidth + Margin) * DrawerOpen;
	const float PY = FMath::Max(16.f, 0.5f * (Canvas->ClipY - PanelH));
	DrawerRect = { PX, PY, DrawerWidth, PanelH };
	HandleRect = { PX - HandleW, 0.5f * (Canvas->ClipY - HandleH), HandleW + 14.f, HandleH };

	// Tutamak: panelin solunda yuvarlak sekme; ev simgesi, harf harf "BINALAR", yon oku. Ustune gelince
	// biraz disari cikar ve turkuaza boyanir.
	const bool bHandleHover = Hover(HandleRect);
	const float HandleNudge = bHandleHover ? 4.f : 0.f;
	Card(HandleRect.X - HandleNudge, HandleRect.Y, HandleRect.W + HandleNudge, HandleRect.H, 14.f,
		bHandleHover ? FMath::Lerp(PanelColor, Pale(Teal), 0.5f) : PanelColor);
	DrawHouseIcon(HandleRect.X + 0.5f * HandleW, HandleRect.Y + 20.f, 16.f, Teal);
	const FString Word = TEXT("BİNALAR");
	const float LetterH = (HandleH - 56.f) / Word.Len();
	for (int32 I = 0; I < Word.Len(); ++I)
	{
		const FString Ch = Word.Mid(I, 1);
		const FVector2D Sz = Measure(Ch, 0.6f, false);
		Text(Ch, HandleRect.X + 0.5f * (HandleW - Sz.X), HandleRect.Y + 36.f + LetterH * I, Ink, 0.6f);
	}
	{
		// Ok: kapaliyken "<" (cek), acikken ">" (it).
		const float Wig = bHandleHover ? 2.f * FMath::Sin(NowReal * 8.f) : 0.f;
		const float AX = HandleRect.X + 0.5f * HandleW + Wig, AY = HandleRect.Y + HandleH - 14.f;
		const float D = bDrawerWanted ? 3.f : -3.f;
		const FLinearColor AC = bHandleHover ? Teal : Muted;
		DrawLine(AX - D, AY - 5.f, AX + D, AY, AC, 2.f);
		DrawLine(AX + D, AY, AX - D, AY + 5.f, AC, 2.f);
	}

	CardRects.Reset();
	if (DrawerOpen <= 0.01f)
	{
		return;
	}
	Card(PX, PY, DrawerWidth, PanelH, 20.f, PanelColor);
	// Baslik: ev simgesi + "Binalar" + daraltma oku.
	DrawHouseIcon(PX + 30.f, PY + 30.f, 20.f, Teal);
	const FVector2D HSz = Measure(TEXT("Binalar"), 1.0f, false);
	Text(TEXT("Binalar"), PX + 52.f, PY + 30.f - 0.5f * HSz.Y, Ink, 1.0f);
	{
		// Daraltma oku: tiklayinca cekmece kapanir.
		const float AX = PX + DrawerWidth - 28.f, AY = PY + 30.f;
		HeaderCloseRect = { AX - 16.f, AY - 16.f, 32.f, 32.f };
		const bool bH = Hover(HeaderCloseRect);
		if (bH)
		{
			FillCircle(AX, AY, 15.f, Pale(Teal));
		}
		const FLinearColor AC = bH ? Teal : Muted;
		DrawLine(AX - 3.f, AY - 6.f, AX + 3.f, AY, AC, 2.5f);
		DrawLine(AX + 3.f, AY, AX - 3.f, AY + 6.f, AC, 2.5f);
	}
	float Y = PY + HeaderH;
	for (int32 K = 0; K < N; ++K, Y += CardH + 10.f)
	{
		const int32 I = CardSlots[K];
		const FSLSlot& S = B.Slots[I];
		const bool bOpen = B.IsSlotUnlocked(I);
		const bool bSel = B.SelectedSlot == I && !B.IsMoving();
		const FRect Hit{ PX + 12.f, Y, DrawerWidth - 24.f, CardH };
		CardRects.Add(Hit);
		const bool bHov = Hover(Hit);
		// Basilinca 0.12 sn icin kuculur; kilitli karta tiklayinca yana sallanir; acik kart ustundeyken kalkar.
		const float PressT = (PressedCard == K) ? (NowReal - PressedAt) / 0.14f : 1.f;
		const float Squash = PressT < 1.f ? 1.f - 0.05f * FMath::Sin(PressT * UE_PI) : 1.f;
		const float ShakeT = (ShakeCard == K) ? NowReal - ShakeAt : 10.f;
		const float Shake = ShakeT < 0.4f ? 6.f * FMath::Sin(ShakeT * 45.f) * (1.f - ShakeT / 0.4f) : 0.f;
		const float Lift = (bHov && bOpen) ? 3.f : 0.f;
		FRect R{ Hit.X + Shake + 0.5f * Hit.W * (1.f - Squash), Hit.Y - Lift + 0.5f * Hit.H * (1.f - Squash), Hit.W * Squash, Hit.H * Squash };
		if (bHov && bOpen)
		{
			FillRound(R.X + 1.f, R.Y + 6.f, R.W, R.H, 14.f, FLinearColor(0.30f, 0.20f, 0.40f, 0.12f)); // kalkan kartin golgesi
		}
		if (bSel || (bHov && bOpen))
		{
			FillRound(R.X - 2.f, R.Y - 2.f, R.W + 4.f, R.H + 4.f, 16.f, bSel ? Teal : Pale(Teal)); // secili / ustunde
		}
		FillRound(R.X, R.Y, R.W, R.H, 14.f, bOpen ? FLinearColor::White : FLinearColor(0.95f, 0.94f, 0.93f));

		const float Thumb = CardH - 16.f;
		DrawSlotThumb(S, R.X + 8.f, R.Y + 8.f, Thumb, bOpen);

		const float TX = R.X + Thumb + 20.f;
		const FLinearColor TextC = bOpen ? Ink : Muted;
		Text(S.Label, TX, R.Y + 10.f, TextC, 0.9f);
		const int32 Left = B.GetRemaining(S.Id);
		if (Left >= 0)
		{
			const FString L = FString::Printf(TEXT("x%d"), Left);
			Text(L, TX + Measure(S.Label, 0.9f, false).X + 8.f, R.Y + 13.f, Left > 0 ? Teal : Red, 0.7f);
		}
		Text(S.Mass < 1.f ? FString::Printf(TEXT("Kütle %.1f"), S.Mass) : FString::Printf(TEXT("Kütle %.0f"), S.Mass), TX, R.Y + 36.f, Muted, 0.7f);
		// Izgara simgesi (2x2 kucuk kare) + ayak izi.
		const float GX = TX, GY = R.Y + 62.f;
		for (int32 Q = 0; Q < 4; ++Q)
		{
			DrawRect(Muted, GX + (Q % 2) * 6.f, GY + (Q / 2) * 6.f, 5.f, 5.f);
		}
		Text(FString::Printf(TEXT("%dx%d"), S.SizeCells.X, S.SizeCells.Y), GX + 16.f, GY - 3.f, Muted, 0.65f);

		// Sag: acik kartta turkuaz "Yerlestir" dugmesi, kilitlide kilit + "Kilitli".
		const float BtnW = 88.f, BtnH = 30.f;
		const float BX = R.X + R.W - BtnW - 10.f, BY = R.Y + R.H - BtnH - 10.f;
		if (bOpen)
		{
			const bool bBtn = FRect{ BX, BY, BtnW, BtnH }.Contains(MousePos);
			const FLinearColor BtnC = bBtn ? Teal * 0.82f : bHov ? Teal * 0.92f : Teal;
			FillRound(BX, BY, BtnW, BtnH, 0.5f * BtnH, FLinearColor(BtnC.R, BtnC.G, BtnC.B, 1.f));
			const FVector2D BSz = Measure(TEXT("Yerleştir"), 0.65f, false);
			Text(TEXT("Yerleştir"), BX + 0.5f * (BtnW - BSz.X), BY + 0.5f * (BtnH - BSz.Y), FLinearColor::White, 0.65f);
			const FString Key = FString::Printf(TEXT("%d"), I + 1);
			const FVector2D KSz = Measure(Key, 0.6f, false);
			Text(Key, R.X + R.W - 14.f - KSz.X, R.Y + 8.f, Muted, 0.6f); // klavye kisayolu
		}
		else
		{
			DrawLockIcon(BX + 22.f, BY + 0.5f * BtnH, 18.f, Muted);
			Text(TEXT("Kilitli"), BX + 36.f, BY + 0.5f * BtnH - 0.5f * Measure(TEXT("Kilitli"), 0.65f, false).Y, Muted, 0.65f);
		}
	}
}

void ASLHUD::DrawCenterMessage(const ASLPlatform& P, bool bLevelComplete)
{
	FString S;
	FLinearColor C;
	if (bLevelComplete)
	{
		const ASLGameMode* GM = GetWorld()->GetAuthGameMode<ASLGameMode>();
		const ASLLevelDirector* D = GM ? GM->GetDirector() : nullptr;
		S = (D && D->NextLevelNumber > 0)
			? FString::Printf(TEXT("BÖLÜM TAMAMLANDI   Enter: %d. bölüm   Space: tekrar"), D->NextLevelNumber)
			: FString(TEXT("BÖLÜM TAMAMLANDI   Space: tekrar"));
		C = Teal;
	}
	else
	{
		const ASLGameMode* GMo = GetWorld()->GetAuthGameMode<ASLGameMode>();
		if (GMo && GMo->IsOutOfMoves())
		{
			S = TEXT("HAMLE BİTTİ   Space / F5: tekrar");
			C = Red;
		}
		else switch (P.BalanceState)
		{
		case ESLBalanceState::Warning:   S = TEXT("DENGE SINIRDA"); C = Yellow; break;
		case ESLBalanceState::Failing:   S = FString::Printf(TEXT("DEVRİLİYOR  %.1f"), FMath::Max(0.f, P.FailSeconds - P.FailTimer)); C = Red; break;
		case ESLBalanceState::Collapsed: S = TEXT("DEVRİLDİ   Space / F5: tekrar"); C = Red; break;
		default: return;
		}
	}
	const float Scale = 1.3f;
	const FVector2D Sz = Measure(S, Scale, true);
	const float X = 0.5f * (Canvas->ClipX - Sz.X), Y = Canvas->ClipY - 64.f - Sz.Y;
	const float BH = Sz.Y + 24.f;
	const ASLGameMode* GM = GetWorld()->GetAuthGameMode<ASLGameMode>();
	const ASLLevelDirector* Dir = GM ? GM->GetDirector() : nullptr;
	const bool bOver = GM && GM->IsRoundOver();
	const bool bNext = bLevelComplete && Dir && Dir->NextLevelNumber > 0;
	const float Lift = bOver ? 44.f : 0.f; // altta dugmelere yer
	Card(X - 54.f, Y - 12.f - Lift, Sz.X + 84.f, BH, 0.5f * BH, PanelColor);
	FillCircle(X - 28.f, Y - 12.f - Lift + 0.5f * BH, 9.f, C);
	Text(S, X, Y - Lift, C == Yellow ? FLinearColor(0.75f, 0.52f, 0.05f) : C, Scale, true);
	AgainRect = {};
	NextRect = {};
	if (bOver)
	{
		// Tiklanir dugmeler: "Tekrar" ve (bolum bittiyse) "Sonraki bölüm". Ustune gelince koyulasir, buyur.
		auto Button = [&](FRect& Out, float BX, const FString& Label, const FLinearColor& Fill)
		{
			Out = { BX, Y + 10.f, 170.f, 40.f };
			const bool bH = Hover(Out);
			const float G = bH ? 3.f : 0.f;
			const FLinearColor F = bH ? Fill * 0.85f : Fill;
			Card(Out.X - G, Out.Y - G, Out.W + 2.f * G, Out.H + 2.f * G, 20.f, FLinearColor(F.R, F.G, F.B, 1.f));
			const FVector2D LS = Measure(Label, 0.8f, false);
			Text(Label, Out.X + 0.5f * (Out.W - LS.X), Out.Y + 0.5f * (Out.H - LS.Y), FLinearColor::White, 0.8f);
		};
		const float Mid = 0.5f * Canvas->ClipX;
		if (bNext)
		{
			Button(AgainRect, Mid - 180.f, TEXT("Tekrar"), Muted);
			Button(NextRect, Mid + 10.f, FString::Printf(TEXT("%d. bölüm"), Dir->NextLevelNumber), Teal);
		}
		else
		{
			Button(AgainRect, Mid - 85.f, TEXT("Tekrar"), bLevelComplete ? Teal : Red);
		}
	}
}

void ASLHUD::DrawTestPanel(const ASLPlatform& P, const USLBuildComponent& B)
{
	TGuardValue<bool> Shadow(bTextShadow, true);
	const float X = 24.f;
	float Y = Canvas->ClipY - 22.f * TextScale * 7.f - 30.f;
	DrawRect(Panel, X - 10.f, Y - 8.f, 600.f, 22.f * TextScale * 7.f + 16.f);
	Line(X, Y, TEXT("TEST PANELİ (H)"), FLinearColor(1.f, 0.9f, 0.5f));
	if (B.State != ESLBuildState::Idle)
	{
		const bool bOk = B.State == ESLBuildState::Placing;
		Line(X, Y, FString::Printf(TEXT("Hücre (%d, %d)  seviye %d  yön %d  %s%s"), B.GetGhostCellMin().X, B.GetGhostCellMin().Y,
			B.GetGhostLevel(), B.GetGhostFacing(), bOk ? TEXT("geçerli") : TEXT("GEÇERSİZ: "), bOk ? TEXT("") : B.GetBlockedReason()),
			bOk ? Green : Red);
	}
	else
	{
		Line(X, Y, TEXT("Elde bir şey yok"), Dim);
	}
	Line(X, Y, FString::Printf(TEXT("Eğim %.1f°  hedef %.1f°  sallanma dahil %.1f°  tahmin %.1f°"),
		P.CurrentTilt.Size(), P.TargetTiltDegrees, P.CurrentTiltDegrees, PredictPlacement(P, B)));
	Line(X, Y, FString::Printf(TEXT("CoM (%.0f, %.0f) cm  |%.0f|  toplam kütle %.1f  nesne %d"),
		P.CenterOfMass.X, P.CenterOfMass.Y, P.CenterOfMass.Size(), P.TotalMass, B.PlacedCount));
	Line(X, Y, FString::Printf(TEXT("PlatformMass %.0f  TiltPerCm %.3f  uyarı %.0f°  devrilme %.0f° / %.1f sn"),
		P.PlatformMass, P.TiltPerCm, P.WarnTilt, P.FailTilt, P.FailSeconds), Dim);
	Line(X, Y, TEXT("Backspace geri al  F5 baştan  Tab çekmece  ~ konsol: SLKnobs, SLSet"), Dim);
	Line(X, Y, TEXT("SLTestLevel1: bölümü otomatik oynar ve ölçer"), Dim);
}

// --- Fare ----------------------------------------------------------------------------------------

int32 ASLHUD::FamilyUnderMouse(const ASLLevelDirector& D) const
{
	// Aileler collision'siz: ekranda govde ortasina en yakin, yakinlastikca buyuyen yaricap icinde.
	const ASLCameraPawn* Cam = Cast<ASLCameraPawn>(GetOwningPawn());
	const float R = FMath::Clamp(30.f * 2900.f / FMath::Max(Cam ? Cam->Distance : 2900.f, 1.f), 18.f, 70.f);
	int32 Best = INDEX_NONE;
	float BestD = R * R;
	for (int32 I = 0; I < D.Families.Num(); ++I)
	{
		const ASLFamily* F = D.GetFamilyActor(I);
		if (!F || F->IsHidden())
		{
			continue;
		}
		const FVector S = const_cast<ASLHUD*>(this)->Project(F->GetClickAnchor());
		const float Dist = FVector2D::DistSquared(FVector2D(S.X, S.Y), MousePos);
		if (S.Z > 0.f && Dist < BestD)
		{
			BestD = Dist;
			Best = I;
		}
	}
	return Best;
}

void ASLHUD::DrawBubble(const FString& S, const FVector& WorldAnchor, float MaxW, float Scale, const FLinearColor& Fill)
{
	const FVector Screen = Project(WorldAnchor);
	if (Screen.Z <= 0.f)
	{
		return;
	}
	const TArray<FString> Lines = Wrap(S, MaxW, Scale);
	const float LineH = Measure(TEXT("Ag"), Scale, false).Y + 2.f;
	float W = 0.f;
	for (const FString& L : Lines)
	{
		W = FMath::Max(W, Measure(L, Scale, false).X);
	}
	const float Pad = 10.f, TailH = 10.f;
	const float BW = W + 2.f * Pad, BH = LineH * Lines.Num() + 2.f * Pad;
	const float BX = FMath::Clamp(Screen.X - 0.5f * BW, 12.f, Canvas->ClipX - BW - 12.f);
	const float BY = FMath::Clamp(Screen.Y - BH - TailH, 12.f, Canvas->ClipY - BH - 12.f);
	OutlineRound(BX, BY, BW, BH, 10.f, 1.f, FLinearColor(0.62f, 0.573f, 0.745f), Fill);
	const float TX = FMath::Clamp(Screen.X, BX + 16.f, BX + BW - 16.f);
	for (int32 I = 0; I < int32(TailH); ++I)
	{
		const float Wd = 14.f * (1.f - float(I) / TailH);
		DrawRect(Fill, TX - 0.5f * Wd, BY + BH + I, Wd, 1.f);
	}
	for (int32 I = 0; I < Lines.Num(); ++I)
	{
		Text(Lines[I], BX + Pad, BY + Pad + I * LineH, Ink, Scale);
	}
}

void ASLHUD::DrawWishCard(const ASLLevelDirector& D, int32 Fam)
{
	const ASLFamily* F = D.GetFamilyActor(Fam);
	if (!F || F->IsHidden())
	{
		return;
	}
	const FSLFamilyDef& Def = D.Families[Fam];
	const FVector S = Project(F->GetClickAnchor());
	const float W = 300.f;
	const float RowH = 26.f;
	const float H = 58.f + RowH * Def.Needs.Num() + 30.f;
	const float X = FMath::Clamp(S.X + 40.f, 12.f, Canvas->ClipX - W - 12.f);
	const float Y = FMath::Clamp(S.Y - 0.5f * H, 140.f, Canvas->ClipY - H - 12.f);
	Card(X, Y, W, H, 18.f, PanelColor);
	DrawFace(X + 26.f, Y + 26.f, 13.f, D.GetHome(Fam) ? D.GetHappiness(Fam) : 0.6f);
	Text(Def.Name, X + 48.f, Y + 14.f, Ink, 0.85f);
	float RY = Y + 50.f;
	for (int32 N = 0; N < Def.Needs.Num(); ++N)
	{
		const bool bHome = D.GetHome(Fam) != nullptr;
		const bool bMet = bHome && D.IsNeedMet(Fam, N);
		FillCircle(X + 24.f, RY + 9.f, 6.f, !bHome ? FLinearColor(0.80f, 0.78f, 0.80f) : bMet ? Teal : Red);
		Text(Def.Needs[N].Label, X + 40.f, RY, bMet ? Muted : Ink, 0.7f);
		RY += RowH;
	}
	const FString Foot = D.GetHome(Fam) ? D.GetFamilyStatus(Fam) : FString(TEXT("Ev kur ya da boş bir eve tıkla"));
	Text(Foot, X + 20.f, RY + 4.f, Teal, 0.62f);
}

void ASLHUD::DrawFamilyOverlays(ASLLevelDirector& D)
{
	const int32 Hov = FamilyUnderMouse(D);
	D.SetHoveredFamily(Hov);
	bAnyHover |= Hov != INDEX_NONE;
	for (int32 I = 0; I < D.Families.Num(); ++I)
	{
		const ASLFamily* F = D.GetFamilyActor(I);
		if (!F || F->IsHidden())
		{
			continue;
		}
		FString Line;
		if (D.GetFamilyBubble(I, Line))
		{
			DrawBubble(Line, F->GetSpeechAnchor(), 240.f, 0.68f, FLinearColor(1.f, 1.f, 1.f, 0.97f));
		}
		else if (!D.IsFamilyDiscovered(I))
		{
			// "?" rozeti: istekleri bilinmiyor, tikla. Hafif ziplar.
			const FVector S = Project(F->GetSpeechAnchor() - F->GetActorUpVector() * 20.f);
			if (S.Z > 0.f)
			{
				const float Bob = 4.f * FMath::Sin(NowReal * 3.f + I);
				const float R = (Hov == I ? 14.f : 11.f) * UiScale();
				FillCircle(S.X, S.Y + Bob, R, FLinearColor(0.984f, 0.973f, 0.992f, 0.95f));
				Arc(S.X, S.Y + Bob, R, NowReal * 40.f, NowReal * 40.f + 360.f, 1.5f, FLinearColor(0.243f, 0.212f, 0.345f), true);
				const FVector2D Q = Measure(TEXT("?"), 0.7f * UiScale(), false);
				Text(TEXT("?"), S.X - 0.5f * Q.X, S.Y + Bob - 0.5f * Q.Y, FLinearColor(0.243f, 0.212f, 0.345f), 0.7f * UiScale());
			}
		}
	}
	if (D.GetSelectedFamily() != INDEX_NONE)
	{
		DrawLens(D, D.GetSelectedFamily());
	}
}

bool ASLHUD::Hover(const FRect& R)
{
	const bool bH = R.Contains(MousePos);
	bAnyHover |= bH;
	return bH;
}

bool ASLHUD::HandleMouseDown(const FVector2D& Mouse)
{
	ASLGameMode* GM = GetWorld()->GetAuthGameMode<ASLGameMode>();
	ASLPlayerController* PC = Cast<ASLPlayerController>(GetOwningPlayerController());
	for (int32 I = 0; I < MenuRects.Num(); ++I)
	{
		if (MenuRects[I].Contains(Mouse) && MenuActions.IsValidIndex(I))
		{
			DoMenuAction(MenuActions[I]);
			return true;
		}
	}
	if (GM && GM->IsMenu())
	{
		if (MusicRect.Contains(Mouse))
		{
			GM->ToggleMusic();
			return true;
		}
		if (MenuPage == ESLMenuPage::Credits)
		{
			MenuPage = ESLMenuPage::Main; // jenerikte tiklama geri doner
		}
		return true; // menude tahtaya tiklama oyuna gecmez
	}
	if (MusicRect.W > 0.f && MusicRect.Contains(Mouse) && GM)
	{
		GM->ToggleMusic();
		ASLGameMode::PlaySfx(this, TEXT("SFX_UI_Tik"), 0);
		return true;
	}
	if (GM && GM->IsRoundOver())
	{
		const ASLLevelDirector* Dir = GM->GetDirector();
		if (NextRect.W > 0.f && NextRect.Contains(Mouse))
		{
			if (NextAction == 0 && Dir && Dir->NextLevelNumber > 0)
			{
				GM->TravelToLevel(Dir->NextLevelNumber);
			}
			else if (NextAction != 0)
			{
				DoMenuAction(NextAction);
			}
			return true;
		}
		if (AgainRect.W > 0.f && AgainRect.Contains(Mouse))
		{
			GM->TravelToLevel(GM->LevelNumber);
			return true;
		}
	}
	if (PC && (RotLeftRect.Contains(Mouse) || RotRightRect.Contains(Mouse)))
	{
		PC->RotateView(RotLeftRect.Contains(Mouse) ? -1 : 1);
		return true;
	}
	if (ASLLevelDirector* Dir = GM ? GM->GetDirector() : nullptr)
	{
		for (int32 I = 0; I < FamilyRowRects.Num(); ++I)
		{
			if (FamilyRowRects[I].Contains(Mouse) && Dir->IsFamilyPresent(I))
			{
				Dir->SelectFamily(I); // takimyildizindan da secilir
				return true;
			}
		}
		const int32 Fam = FamilyUnderMouse(*Dir);
		if (Fam != INDEX_NONE)
		{
			Dir->SelectFamily(Fam);
			return true;
		}
	}
	for (int32 K = 0; K < CardRects.Num(); ++K)
	{
		if (!CardRects[K].Contains(Mouse) || !CardSlots.IsValidIndex(K))
		{
			continue;
		}
		const int32 I = CardSlots[K];
		USLBuildComponent* B = PC ? PC->Build.Get() : nullptr;
		if (!B)
		{
			return true;
		}
		if (!B->IsSlotUnlocked(I) || B->GetRemaining(B->Slots[I].Id) == 0)
		{
			ShakeCard = K;
			ShakeAt = NowReal;
			Flash(B->IsSlotUnlocked(I) ? FString(TEXT("Bu binadan kalmadı: var olanı tıklayıp taşıyabilirsin."))
				: FString(TEXT("Bu bina henüz kilitli: ailenin istediği görevi bitirince açılır.")));
			ASLGameMode::PlaySfx(this, TEXT("SFX_Gecersiz"), 0, 0.8f);
			return true;
		}
		PressedCard = K;
		PressedAt = NowReal;
		B->SelectSlot(I);
		ASLGameMode::PlaySfx(this, TEXT("SFX_UI_Tik"), 0);
		return true;
	}
	return false;
}

void ASLHUD::HandleMouseUp(const FVector2D& Mouse)
{
	if (!bDragging)
	{
		return;
	}
	bDragging = false;
	// Tiklama (surukleme degil): ac/kapa. Surukleme: yarisini gectiyse acik kalir.
	SetDrawerOpen(bDragMoved ? DrawerOpen > 0.4f : !bDrawerWanted);
}

// =================================================================================================
// "Gokyuzu pusulasi" (16 Eki, Andac C yonunu secti; tasarim: https://claude.ai/artifact/FNFkufSuNDvnZLMMMCkqhp)
// Panel/kart yok: ince cizgiler, daireler, aralikli buyuk harfli basliklar. Ustte gunesin yolu ve hamle
// boncuklari, solda aileler takimyildizi, sol altta denge pusulasi (su terazisi gibi: nokta yatilan yone
// kayar, ic halka uyari, dis halka devrilme) ve cevresinde binalar, secili ailede mercek + istek kutulari.
// =================================================================================================

namespace
{
	const FLinearColor CInk(0.243f, 0.212f, 0.345f);
	const FLinearColor CMuted(0.416f, 0.376f, 0.525f);
	const FLinearColor CLine(0.62f, 0.573f, 0.745f);
	const FLinearColor CLens(0.949f, 0.929f, 0.965f);
	const FLinearColor CCream(0.984f, 0.973f, 0.992f);
	const FLinearColor CSun(0.94f, 0.75f, 0.38f);
	const FLinearColor CWarn(0.85f, 0.65f, 0.29f);
	const FLinearColor CFail(0.85f, 0.40f, 0.37f);
	const FLinearColor CGood(0.18f, 0.48f, 0.44f);
	const FLinearColor CFailText(0.66f, 0.26f, 0.23f);

	/** FString::ToUpper Turkce harfleri atliyordu ("KOMşULAR", "DURAĞı"; ekran goruntusu). */
	FString TrUpper(const FString& In)
	{
		FString S = In;
		const TCHAR* From[] = { TEXT("ç"), TEXT("ğ"), TEXT("ı"), TEXT("i"), TEXT("ö"), TEXT("ş"), TEXT("ü") };
		const TCHAR* To[] = { TEXT("Ç"), TEXT("Ğ"), TEXT("I"), TEXT("İ"), TEXT("Ö"), TEXT("Ş"), TEXT("Ü") };
		for (int32 I = 0; I < 7; ++I)
		{
			S.ReplaceInline(From[I], To[I], ESearchCase::CaseSensitive);
		}
		return S.ToUpper();
	}

	FVector2D Quad(const FVector2D& A, const FVector2D& C, const FVector2D& B, float T)
	{
		return (1.f - T) * (1.f - T) * A + 2.f * (1.f - T) * T * C + T * T * B;
	}
}

void ASLHUD::Arc(float CX, float CY, float R, float A0, float A1, float Thick, const FLinearColor& C, bool bDashed)
{
	// Acilar derece, ekranda saat yonunun tersi (0 = sag, 90 = yukari).
	const int32 N = FMath::Clamp(FMath::RoundToInt(FMath::Abs(A1 - A0) / 4.f), 4, 120);
	for (int32 I = 0; I < N; ++I)
	{
		if (bDashed && (I % 2))
		{
			continue;
		}
		const float T0 = FMath::DegreesToRadians(FMath::Lerp(A0, A1, float(I) / N));
		const float T1 = FMath::DegreesToRadians(FMath::Lerp(A0, A1, float(I + 1) / N));
		DrawLine(CX + R * FMath::Cos(T0), CY - R * FMath::Sin(T0), CX + R * FMath::Cos(T1), CY - R * FMath::Sin(T1), C, Thick);
	}
}

float ASLHUD::PillAlpha(float Day) const
{
	// Gece arka plan lacivert: soluk krem zemin griye donup murekkep yaziyi bogdu (ekran goruntusu).
	const float Daylight = Ambience.IsValid() ? Ambience->GetDaylight() : 1.f;
	return FMath::Lerp(0.92f, Day, Daylight);
}

void ASLHUD::TextHalo(const FString& S, float X, float Y, const FLinearColor& C, float Scale)
{
	// Panelsiz yazi tahtanin ve adaciklarin ustunde kayboluyordu; 8 yonlu hale bulanik/kirli gorundu
	// (ekran goruntusu). Arkasina cok soluk krem hap: gokyuzunde neredeyse gorunmez, tahtada okutur.
	const FVector2D Sz = Measure(S, Scale, false);
	const float PadX = 8.f, PadY = 2.f;
	FillRound(X - PadX, Y - PadY, Sz.X + 2.f * PadX, Sz.Y + 2.f * PadY, 0.5f * (Sz.Y + 2.f * PadY), FLinearColor(0.984f, 0.973f, 0.992f, PillAlpha(0.62f)));
	Text(S, X, Y, C, Scale);
}

void ASLHUD::TextSpaced(const FString& S, float CenterX, float Y, float Scale, float Spacing, const FLinearColor& C)
{
	// Aralikli buyuk harf (Monument Valley basliklari): harf harf, ortalanmis.
	float W = 0.f;
	TArray<float> Ws;
	TGuardValue<bool> TitleMeasure(bUseTitleFont, true);
	for (int32 I = 0; I < S.Len(); ++I)
	{
		Ws.Add(Measure(S.Mid(I, 1), Scale, false).X);
		W += Ws.Last() + (I + 1 < S.Len() ? Spacing : 0.f);
	}
	float X = CenterX - 0.5f * W;
	const float H = Measure(TEXT("A"), Scale, false).Y;
	TGuardValue<bool> Title(bUseTitleFont, true); // basliklar Cinzel
	FillRound(X - 14.f, Y - 3.f, W + 28.f, H + 6.f, 0.5f * (H + 6.f), FLinearColor(0.984f, 0.973f, 0.992f, PillAlpha(0.5f)));
	for (int32 I = 0; I < S.Len(); ++I)
	{
		Text(S.Mid(I, 1), X, Y, C, Scale);
		X += Ws[I] + Spacing;
	}
}

void ASLHUD::OutlineRound(float X, float Y, float W, float H, float R, float Thick, const FLinearColor& Line, const FLinearColor& Fill)
{
	FillRound(X, Y, W, H, R, Line);
	FillRound(X + Thick, Y + Thick, W - 2.f * Thick, H - 2.f * Thick, FMath::Max(0.f, R - Thick), Fill);
}

void ASLHUD::DrawSky(const ASLLevelDirector& D)
{
	const float S = UiScale();
	const float W = Canvas->ClipX;
	const FVector2D A(0.19f * W, 140.f * S), C(0.5f * W, -40.f * S), B(0.81f * W, 140.f * S);
	// Gunesin yolu: ince egri.
	FVector2D Prev = A;
	for (int32 I = 1; I <= 48; ++I)
	{
		const FVector2D P = Quad(A, C, B, I / 48.f);
		DrawLine(Prev.X, Prev.Y, P.X, P.Y, CLine, 1.f);
		Prev = P;
	}
	// Boncuklar: hamle siniri varsa hamleler (dolu = harcandi), yoksa gunun saatleri (06..21).
	const int32 Limit = D.MoveLimit;
	const int32 Beads = Limit > 0 ? Limit : 16;
	const int32 Used = Limit > 0 ? D.GetMoveCount() : FMath::Clamp(FMath::FloorToInt(FMath::Fmod(D.GetTimeHours(), 24.f)) - 6 + 1, 0, 16);
	for (int32 I = 0; I < Beads; ++I)
	{
		const FVector2D P = Quad(A, C, B, 0.03f + 0.94f * I / FMath::Max(1, Beads - 1));
		if (I < Used)
		{
			FillCircle(P.X, P.Y, 3.f * S, CInk);
		}
		else
		{
			Arc(P.X, P.Y, 3.f * S, 0.f, 360.f, 1.f, CInk);
		}
	}
	// Gunes ya da ay: saate gore yayin uzerinde.
	if (!Ambience.IsValid())
	{
		for (TActorIterator<ASLAmbience> It(GetWorld()); It; ++It)
		{
			Ambience = *It;
			break;
		}
	}
	const float Hour = Ambience.IsValid() ? Ambience->GetShownHour() : FMath::Fmod(D.GetTimeHours(), 24.f);
	const bool bDay = (Ambience.IsValid() ? Ambience->GetDaylight() : ASLAmbience::DaylightAt(Hour)) >= 0.35f;
	const float T = bDay ? FMath::Clamp((Hour - 5.f) / 15.f, 0.f, 1.f) : FMath::Clamp(FMath::Fmod(Hour + 24.f - 19.f, 24.f) / 12.f, 0.f, 1.f);
	const FVector2D Sun = Quad(A, C, B, T);
	if (bDay)
	{
		FillCircle(Sun.X, Sun.Y, 13.f * S, CSun);
		Arc(Sun.X, Sun.Y, 20.f * S, 0.f, 360.f, 1.f, CSun);
	}
	else
	{
		FillCircle(Sun.X, Sun.Y, 12.f * S, FLinearColor(0.78f, 0.80f, 0.98f));
		FillCircle(Sun.X + 5.f * S, Sun.Y - 4.f * S, 10.f * S, FLinearColor(0.86f, 0.83f, 0.93f, 0.95f));
	}

	// Baslik: bolum adi aralikli buyuk harf, altinda gun / saat / hamle.
	FString Title = D.LevelTitle;
	int32 Colon = INDEX_NONE;
	if (Title.FindChar(TEXT(':'), Colon))
	{
		Title = Title.Mid(Colon + 1).TrimStartAndEnd();
	}
	const float TitleY = 92.f * S;
	TextSpaced(TrUpper(Title), 0.5f * W, TitleY, 1.25f * S, 7.f * S, CInk);
	const int32 DayN = FMath::FloorToInt(D.GetTimeHours() / 24.f) + 1;
	FString Sub = FString::Printf(TEXT("%d. gün  ·  %02d:%02d"), DayN, FMath::FloorToInt(Hour), FMath::FloorToInt(FMath::Frac(Hour) * 60.f));
	if (Limit > 0)
	{
		Sub += FString::Printf(TEXT("  ·  %d hamle"), D.GetMovesLeft());
	}
	const FVector2D SubSz = Measure(Sub, 0.72f * S, false);
	TextHalo(Sub, 0.5f * W - 0.5f * SubSz.X, TitleY + 34.f * S, (Limit > 0 && D.GetMovesLeft() <= 3) ? CFailText : CMuted, 0.72f * S);

	// Egitim bolumu: siradaki gorev basligin altinda tek satir + ilerleme noktalari.
	if (!D.bOpenLevel && D.Tasks.IsValidIndex(D.CurrentTask) && !D.bLevelComplete)
	{
		FString Task = D.Tasks[D.CurrentTask].Text;
		if (D.ProgressGoal > 1)
		{
			Task += FString::Printf(TEXT("  %d/%d"), D.Progress, D.ProgressGoal);
		}
		const FVector2D TS = Measure(Task, 0.82f * S, false);
		TextHalo(Task, 0.5f * W - 0.5f * TS.X, TitleY + 62.f * S, CInk, 0.82f * S);
		const float DotY = TitleY + 92.f * S, Gap = 14.f * S;
		const float DX0 = 0.5f * W - 0.5f * Gap * (D.Tasks.Num() - 1);
		for (int32 I = 0; I < D.Tasks.Num(); ++I)
		{
			if (D.IsTaskDone(I))
			{
				FillCircle(DX0 + I * Gap, DotY, 3.f * S, CInk);
			}
			else
			{
				Arc(DX0 + I * Gap, DotY, 3.f * S, 0.f, 360.f, 1.f, I == D.CurrentTask ? CInk : CLine);
			}
		}
	}

	// Muzik: sag ustte ince hoparlor.
	MusicRect = { W - 64.f * S, 28.f * S, 44.f * S, 44.f * S };
	const bool bH = Hover(MusicRect);
	const ASLGameMode* GM = GetWorld()->GetAuthGameMode<ASLGameMode>();
	if (bH)
	{
		Arc(MusicRect.X + 22.f * S, MusicRect.Y + 22.f * S, 20.f * S, 0.f, 360.f, 1.f, CLine);
	}
	DrawSpeakerIcon(MusicRect.X + 20.f * S, MusicRect.Y + 22.f * S, 22.f * S, GM && GM->IsMusicOn(), bH ? CInk : CMuted);
}

void ASLHUD::DrawConstellation(ASLLevelDirector& D)
{
	if (!D.HasFamilyNeeds())
	{
		FamilyRowRects.Reset();
		return;
	}
	const float S = UiScale();
	const float X = 64.f * S, Y0 = 250.f * S, Row = 56.f * S;
	FamilyRowRects.SetNum(D.Families.Num());
	// Dugumleri birlestiren soluk cizgi: takimyildizi.
	for (int32 I = 0; I + 1 < D.Families.Num(); ++I)
	{
		DrawLine(X, Y0 + I * Row, X + (I % 2 ? -6.f : 6.f) * S, Y0 + (I + 1) * Row, FLinearColor(CLine.R, CLine.G, CLine.B, 0.6f), 1.f);
	}
	for (int32 I = 0; I < D.Families.Num(); ++I)
	{
		const FSLFamilyDef& Def = D.Families[I];
		const float CY = Y0 + I * Row, CX = X + (I % 2 ? 6.f : 0.f) * S;
		const bool bPresent = D.IsFamilyPresent(I);
		const bool bKnown = D.IsFamilyDiscovered(I);
		const ASLPlaceable* Home = D.GetHome(I);
		const bool bHappy = D.IsFamilyHappy(I);
		const bool bSel = D.GetSelectedFamily() == I;
		FRect R{ CX - 16.f * S, CY - 20.f * S, 230.f * S, 40.f * S };
		FamilyRowRects[I] = R;
		const bool bHov = bPresent && Hover(R);
		const float NR = 8.f * S;
		if (bSel || bHov)
		{
			Arc(CX, CY, 16.f * S, 0.f, 360.f, 1.f, CInk);
		}
		FString Status;
		FLinearColor StatusC = CMuted;
		if (!bPresent)
		{
			Arc(CX, CY, NR, 0.f, 360.f, 1.f, CLine, true);
			Status = Def.bArriveAtStop ? TEXT("otobüsle gelecek") : TEXT("yolda");
		}
		else if (!bKnown)
		{
			Arc(CX, CY, NR, 0.f, 360.f, 1.f, CInk, true);
			const FVector2D Q = Measure(TEXT("?"), 0.55f * S, false);
			Text(TEXT("?"), CX - 0.5f * Q.X, CY - 0.5f * Q.Y, CInk, 0.55f * S);
			Status = TEXT("tanışmadın");
		}
		else if (!Home)
		{
			Arc(CX, CY, NR, 0.f, 360.f, 2.f, Def.Tint);
			Status = TEXT("ev arıyor");
		}
		else if (bHappy)
		{
			FillCircle(CX, CY, NR, Def.Tint);
			Status = TEXT("mutlu");
			StatusC = CGood;
		}
		else
		{
			FillCircle(CX, CY, NR + 6.f * S, FLinearColor(CFail.R, CFail.G, CFail.B, 0.18f));
			Arc(CX, CY, NR, 0.f, 360.f, 2.f, CFail);
			Status = Home->bResidentsArrived ? TEXT("mutsuz") : TEXT("taşınıyor");
			StatusC = Home->bResidentsArrived ? CFailText : CMuted;
		}
		FString Short = Def.Name;
		int32 Sp = INDEX_NONE;
		if (Short.FindChar(TEXT(' '), Sp))
		{
			Short = Short.Left(Sp);
		}
		const float TS = 0.8f * S;
		const FVector2D NS = Measure(Short, TS, false);
		const float TX = CX + 22.f * S, TY = CY - 0.5f * NS.Y;
		TextHalo(Short, TX, TY, bPresent ? CInk : CMuted, TS);
		if (bSel)
		{
			Text(Short, TX + 0.6f, TY, CInk, TS); // secili: kalinlastir
		}
		TextHalo(Status, TX + NS.X + 10.f * S, CY - 0.5f * Measure(Status, 0.66f * S, false).Y, StatusC, 0.66f * S);
	}
}

void ASLHUD::DrawLens(const ASLLevelDirector& D, int32 Fam)
{
	const ASLFamily* F = D.GetFamilyActor(Fam);
	if (!F || F->IsHidden())
	{
		return;
	}
	const float S = UiScale();
	const FSLFamilyDef& Def = D.Families[Fam];
	const FVector P = Project(F->GetClickAnchor());
	if (P.Z <= 0.f)
	{
		return;
	}
	const float LR = 34.f * S;
	Arc(P.X, P.Y, LR, 0.f, 360.f, 1.5f, CInk);
	// Kutular sagda yukarida; ekrandan tasarsa sola.
	const int32 N = Def.Needs.Num();
	const float BoxW = 178.f * S, BoxH = 74.f * S, Gap = 10.f * S;
	const float PanelW = N * BoxW + FMath::Max(0, N - 1) * Gap;
	float BX = P.X + LR + 90.f * S;
	if (BX + PanelW > Canvas->ClipX - 24.f * S)
	{
		BX = P.X - LR - 90.f * S - PanelW;
	}
	const float BY = FMath::Clamp(P.Y - 120.f * S, 170.f * S, Canvas->ClipY - BoxH - 60.f * S);
	const bool bLeft = BX < P.X;
	const FVector2D From(P.X + (bLeft ? -0.75f : 0.75f) * LR, P.Y - 0.6f * LR);
	const FVector2D To(bLeft ? BX + PanelW : BX, BY + 46.f * S);
	DrawLine(From.X, From.Y, To.X, To.Y, CInk, 1.f);
	TextSpaced(TrUpper(Def.Name), BX + 0.5f * PanelW, BY, 0.78f * S, 3.f * S, CInk);
	const ASLPlaceable* Home = D.GetHome(Fam);
	for (int32 I = 0; I < N; ++I)
	{
		const FSLNeed& Need = Def.Needs[I];
		bool bMet = false;
		const FString Detail = D.DescribeNeed(Fam, I, bMet);
		const float X = BX + I * (BoxW + Gap), Y = BY + 30.f * S;
		const FLinearColor Edge = !Home ? CLine : bMet ? CGood : CFail;
		OutlineRound(X, Y, BoxW, BoxH, 2.f, 1.f, Edge, FLinearColor(CCream.R, CCream.G, CCream.B, 0.92f));
		const FString Kind = Need.Kind == ESLNeedKind::FarFrom ? TEXT("İSTEMEDİĞİ") : TEXT("İSTEDİĞİ");
		Text(Kind, X + 12.f * S, Y + 8.f * S, !Home ? CMuted : bMet ? CGood : CFailText, 0.56f * S);
		Text(Need.Label, X + 12.f * S, Y + 26.f * S, CInk, 0.74f * S);
		Text(!Home ? FString(TEXT("önce bir ev")) : Detail, X + 12.f * S, Y + 50.f * S, CMuted, 0.6f * S);
	}
	if (!Home)
	{
		TextHalo(TEXT("Bir bina seç ve koy, ya da boş bir eve tıkla"), BX, BY + 30.f * S + BoxH + 10.f * S, CInk, 0.64f * S);
	}
}

void ASLHUD::DrawCompass(const ASLPlatform& P, const USLBuildComponent& B)
{
	const float S = UiScale();
	const float CX = 190.f * S, CY = Canvas->ClipY - 160.f * S, R = 64.f * S;
	FillCircle(CX, CY, R, CLens);
	Arc(CX, CY, R, 0.f, 360.f, 1.f, CLine);
	// Su terazisi: dis halka devrilme, ic halka uyari; nokta platformun yattigi yone, egimle orantili kayar.
	const float Max = FMath::Max(P.FailTilt * 1.15f, 0.1f);
	const float WarnR = R * P.WarnTilt / Max, FailR = R * P.FailTilt / Max;
	Arc(CX, CY, WarnR, 0.f, 360.f, 1.f, CWarn, true);
	Arc(CX, CY, FailR, 0.f, 360.f, 2.f, CFail);
	const FVector2D Tilt = P.CurrentTilt;
	const float Deg = P.IsCollapsed() ? Max : Tilt.Size();
	FVector2D Dir(0.f, -1.f);
	if (Tilt.SizeSquared() > 1e-4f)
	{
		// Platform yerelindeki yatis yonunu ekrana tasi: iki noktanin izdusumu.
		const FVector C0 = P.TiltPivot->GetComponentLocation();
		const FVector C1 = C0 + FVector(Tilt.X, Tilt.Y, 0.f).GetSafeNormal() * 100.f;
		const FVector S0 = Project(C0), S1 = Project(C1);
		Dir = FVector2D(S1.X - S0.X, S1.Y - S0.Y).GetSafeNormal();
	}
	const float Len = R * FMath::Clamp(Deg / Max, 0.f, 1.f);
	const FVector2D Tip(CX + Dir.X * Len, CY + Dir.Y * Len);
	const FLinearColor TipC = Deg >= P.FailTilt ? CFail : Deg >= P.WarnTilt ? CWarn : CInk;
	DrawLine(CX, CY, Tip.X, Tip.Y, TipC, 2.f);
	FillCircle(CX, CY, 4.f * S, CInk);
	FillCircle(Tip.X, Tip.Y, 6.f * S, TipC);
	// Tahmin: elindekini koyarsan nokta nereye gider (bos halka).
	if (const ASLPlaceable* G = B.GetGhost(); G && B.State == ESLBuildState::Placing)
	{
		const float Pred = P.PredictTiltDegrees(G->Mass, B.GetGhostCenterLocal());
		const FLinearColor PC = Pred >= P.FailTilt ? CFail : Pred >= P.WarnTilt ? CWarn : CInk;
		const FVector2D GL = B.GetGhostCenterLocal();
		FVector2D PD = Dir;
		{
			// Tahmini yon: CoM'un hayaletle birlikte gidecegi taraf (kaba: hayaletin oldugu yon).
			const FVector C0 = P.TiltPivot->GetComponentLocation();
			const FVector C1 = P.TiltPivot->GetComponentTransform().TransformPosition(FVector(GL.X, GL.Y, 0.f));
			const FVector S0 = Project(C0), S1 = Project(C1);
			const FVector2D V(S1.X - S0.X, S1.Y - S0.Y);
			if (V.SizeSquared() > 1.f && Pred > Deg)
			{
				PD = (Dir * Deg + V.GetSafeNormal() * (Pred - Deg)).GetSafeNormal();
			}
		}
		const float PL = R * FMath::Clamp(Pred / Max, 0.f, 1.f);
		Arc(CX + PD.X * PL, CY + PD.Y * PL, 7.f * S, 0.f, 360.f, 2.f, PC);
	}
	const FString DegS = FString::Printf(TEXT("%.0f°"), Deg);
	const FVector2D DS = Measure(DegS, 0.68f * S, false);
	Text(DegS, CX - 0.5f * DS.X, CY + 26.f * S, CInk, 0.68f * S);
	const FVector2D LS = Measure(TEXT("DENGE"), 0.5f * S, false);
	Text(TEXT("DENGE"), CX - 0.5f * LS.X, CY + R - 20.f * S, CMuted, 0.5f * S);
	if (P.BalanceState == ESLBalanceState::Failing)
	{
		// Devrilme geri sayimi: dis halkada azalan yay.
		const float Left = FMath::Clamp(1.f - P.FailTimer / FMath::Max(P.FailSeconds, 0.01f), 0.f, 1.f);
		Arc(CX, CY, R + 6.f * S, 90.f, 90.f + 360.f * Left, 3.f, CFail);
	}

	// Binalar: pusulanin ustunde yay boyunca daireler.
	CardRects.Reset();
	CardSlots.Reset();
	const ASLGameMode* GM = GetWorld()->GetAuthGameMode<ASLGameMode>();
	const ASLLevelDirector* Dir2 = GM ? GM->GetDirector() : nullptr;
	TArray<int32> Shown;
	for (int32 I = 0; I < B.Slots.Num(); ++I)
	{
		bool bUsed = !Dir2;
		for (int32 T = 0; Dir2 && T < Dir2->Tasks.Num() && !bUsed; ++T)
		{
			bUsed = Dir2->Tasks[T].Unlocks.Contains(B.Slots[I].Id);
		}
		if (bUsed)
		{
			Shown.Add(I);
		}
	}
	const float TokR = 32.f * S, RingR = R + 64.f * S;
	const float A0 = Shown.Num() > 4 ? 172.f : 160.f, A1 = Shown.Num() > 4 ? 8.f : 20.f;
	for (int32 K = 0; K < Shown.Num(); ++K)
	{
		const int32 I = Shown[K];
		const FSLSlot& Sl = B.Slots[I];
		const float A = FMath::DegreesToRadians(Shown.Num() > 1 ? FMath::Lerp(A0, A1, float(K) / (Shown.Num() - 1)) : 90.f);
		float TX = CX + RingR * FMath::Cos(A), TY = CY - RingR * FMath::Sin(A);
		const FRect Hit{ TX - TokR, TY - TokR, 2.f * TokR, 2.f * TokR };
		CardRects.Add(Hit);
		CardSlots.Add(I);
		const bool bOpen = B.IsSlotUnlocked(I);
		const int32 Left = B.GetRemaining(Sl.Id);
		const bool bEmpty = Left == 0;
		const bool bSel = B.SelectedSlot == I && !B.IsMoving() && B.State != ESLBuildState::Idle;
		const bool bHov = Hover(Hit);
		const float PressT = (PressedCard == K) ? (NowReal - PressedAt) / 0.14f : 1.f;
		const float ShakeT = (ShakeCard == K) ? NowReal - ShakeAt : 10.f;
		TX += ShakeT < 0.4f ? 5.f * FMath::Sin(ShakeT * 45.f) * (1.f - ShakeT / 0.4f) : 0.f;
		const float RR = TokR * (PressT < 1.f ? 1.f - 0.08f * FMath::Sin(PressT * UE_PI) : 1.f) * (bHov && bOpen && !bEmpty ? 1.08f : 1.f);
		if (bSel)
		{
			FillCircle(TX, TY, RR, CInk);
		}
		else if (bOpen && !bEmpty)
		{
			FillCircle(TX, TY, RR, CCream);
			Arc(TX, TY, RR, 0.f, 360.f, bHov ? 2.f : 1.f, bHov ? CInk : CLine);
		}
		else
		{
			Arc(TX, TY, RR, 0.f, 360.f, 1.f, CLine, true);
		}
		const FLinearColor TC = bSel ? CCream : (bOpen && !bEmpty) ? CInk : CMuted;
		const FString L1 = Sl.Label;
		const FString L2 = !bOpen ? FString(TEXT("kilitli")) : bEmpty ? FString(TEXT("bitti")) : Left >= 0 ? FString::Printf(TEXT("%d"), Left) : FString::Printf(TEXT("%d"), I + 1);
		const FVector2D S1 = Measure(L1, 0.68f * S, false), S2 = Measure(L2, 0.58f * S, false);
		Text(L1, TX - 0.5f * S1.X, TY - S1.Y + 1.f, TC, 0.68f * S);
		Text(L2, TX - 0.5f * S2.X, TY + 1.f, TC, 0.58f * S);
	}
}

void ASLHUD::DrawRotateButtons()
{
	// Tek hap, iki yari: sola / saga cevir. Ustune gelinen yari koyulasir.
	const float S = UiScale();
	const float W = 58.f * S, H = 52.f * S;
	const float X = Canvas->ClipX - 2.f * W - 56.f * S, Y = Canvas->ClipY - H - 56.f * S;
	RotLeftRect = { X, Y, W, H };
	RotRightRect = { X + W, Y, W, H };
	const bool bL = Hover(RotLeftRect), bR = Hover(RotRightRect);
	OutlineRound(X, Y, 2.f * W, H, 0.5f * H, 1.f, CLine, CCream);
	if (bL || bR)
	{
		const float HX = bL ? X + 1.f : X + W;
		FillRound(HX, Y + 1.f, W - 1.f, H - 2.f, 0.5f * H - 1.f, CLens);
		DrawRect(CLens, bL ? X + 0.5f * W : X + W, Y + 1.f, 0.5f * W, H - 2.f);
	}
	DrawLine(X + W, Y + 10.f * S, X + W, Y + H - 10.f * S, CLine, 1.f);
	for (int32 Side = 0; Side < 2; ++Side)
	{
		const bool bH = Side == 0 ? bL : bR;
		const float AX = X + (Side + 0.5f) * W, AY = Y + 0.5f * H;
		const float D = Side == 0 ? 5.f : -5.f;
		DrawLine(AX + D * S, AY - 8.f * S, AX - D * S, AY, bH ? CInk : CMuted, 2.f);
		DrawLine(AX - D * S, AY, AX + D * S, AY + 8.f * S, bH ? CInk : CMuted, 2.f);
	}
}

void ASLHUD::DrawHintC(const ASLPlatform& P, const USLBuildComponent& B, const ASLLevelDirector* D)
{
	FString S;
	FLinearColor C;
	BuildHint(P, B, D, S, C);
	LastHint = S;
	if (S.IsEmpty())
	{
		return;
	}
	// Basligin altinda, gokyuzunde: altta tahtanin ustune biniyordu. Egitimde gorev satirinin altinda.
	const float Sc = UiScale();
	const float Scale = 0.76f * Sc;
	const TArray<FString> Lines = Wrap(S, 640.f * Sc, Scale);
	const float LineH = Measure(TEXT("Ag"), Scale, false).Y + 2.f;
	const bool bTask = D && !D->bOpenLevel && !D->bLevelComplete;
	float Y = (bTask ? 210.f : 160.f) * Sc;
	const FLinearColor DC = C.Equals(Teal) ? CLine : C.Equals(Yellow) ? CWarn : CFail;
	for (int32 K = 0; K < Lines.Num(); ++K)
	{
		const FVector2D Sz = Measure(Lines[K], Scale, false);
		const float X = 0.5f * Canvas->ClipX - 0.5f * Sz.X;
		if (K == 0)
		{
			// Renk yalniz kucuk bir elmasla: bilgi / uyari / tehlike.
			const float DX = X - 14.f * Sc, DY = Y + 0.5f * LineH;
			for (int32 I = 0; I < int32(10 * Sc); ++I)
			{
				const float Hw = (I < 5 * Sc ? I : 10 * Sc - I) * 0.9f;
				DrawRect(DC, DX - Hw, DY - 5.f * Sc + I, 2.f * Hw, 1.f);
			}
		}
		TextHalo(Lines[K], X, Y, C.Equals(Teal) ? CInk : (C.Equals(Yellow) ? FLinearColor(0.55f, 0.38f, 0.08f) : CFailText), Scale);
		Y += LineH;
	}
}

void ASLHUD::DrawEndC(const ASLPlatform& P, bool bLevelComplete)
{
	const ASLGameMode* GM = GetWorld()->GetAuthGameMode<ASLGameMode>();
	const ASLLevelDirector* Dir = GM ? GM->GetDirector() : nullptr;
	FString S;
	FLinearColor C = CInk;
	FString Sub;
	if (bLevelComplete)
	{
		S = TEXT("BÖLÜM TAMAMLANDI");
		Sub = Dir && Dir->NextLevelNumber > 0 ? FString(TEXT("Enter: sonraki bölüm  ·  Space: tekrar")) : FString(TEXT("Space: tekrar"));
	}
	else if (GM && GM->IsOutOfMoves())
	{
		S = TEXT("HAMLE BİTTİ");
		C = CFailText;
		Sub = TEXT("Space / F5: tekrar");
	}
	else if (P.BalanceState == ESLBalanceState::Collapsed)
	{
		S = TEXT("DEVRİLDİ");
		C = CFailText;
		Sub = TEXT("Space / F5: tekrar");
	}
	else if (P.BalanceState == ESLBalanceState::Failing)
	{
		S = FString::Printf(TEXT("DEVRİLİYOR  %.1f"), FMath::Max(0.f, P.FailSeconds - P.FailTimer));
		C = CFailText;
	}
	AgainRect = {};
	NextRect = {};
	if (S.IsEmpty())
	{
		return;
	}
	const float Sc = UiScale();
	const float Y = Canvas->ClipY * 0.66f;
	TextSpaced(S, 0.5f * Canvas->ClipX, Y, 1.35f * Sc, 8.f * Sc, C);
	DrawLine(0.5f * Canvas->ClipX - 40.f * Sc, Y + 48.f * Sc, 0.5f * Canvas->ClipX + 40.f * Sc, Y + 48.f * Sc, C, 1.f);
	if (!Sub.IsEmpty())
	{
		const FVector2D SS = Measure(Sub, 0.66f * Sc, false);
		TextHalo(Sub, 0.5f * Canvas->ClipX - 0.5f * SS.X, Y + 58.f * Sc, CMuted, 0.66f * Sc);
	}
	if (!GM || !GM->IsRoundOver())
	{
		return;
	}
	auto Button = [&](FRect& Out, float BX, const FString& Label, bool bPrimary)
	{
		Out = { BX, Y + 92.f * Sc, 168.f * Sc, 46.f * Sc };
		const bool bH = Hover(Out);
		const FLinearColor Fill = bPrimary ? (bH ? CMuted : CInk) : (bH ? CLens : CCream);
		OutlineRound(Out.X, Out.Y, Out.W, Out.H, 0.5f * Out.H, 1.f, bPrimary ? CInk : CLine, Fill);
		const FVector2D LS = Measure(Label, 0.74f * Sc, false);
		Text(Label, Out.X + 0.5f * (Out.W - LS.X), Out.Y + 0.5f * (Out.H - LS.Y), bPrimary ? CCream : CInk, 0.74f * Sc);
	};
	const float Mid = 0.5f * Canvas->ClipX;
	if (bLevelComplete && Dir && Dir->NextLevelNumber > 0)
	{
		NextAction = 0;
		Button(AgainRect, Mid - 178.f * Sc, TEXT("Tekrar"), false);
		Button(NextRect, Mid + 10.f * Sc, FString::Printf(TEXT("%d. bölüm"), Dir->NextLevelNumber), true);
	}
	else if (bLevelComplete)
	{
		NextAction = 22; // son bolum: jenerik
		Button(AgainRect, Mid - 178.f * Sc, TEXT("Tekrar"), false);
		Button(NextRect, Mid + 10.f * Sc, TEXT("Emeği geçenler"), true);
	}
	else
	{
		NextAction = 21;
		Button(AgainRect, Mid - 178.f * Sc, TEXT("Tekrar"), true);
		Button(NextRect, Mid + 10.f * Sc, TEXT("Menü"), false);
	}
}

void ASLHUD::DrawHomeNodes(const ASLLevelDirector& D)
{
	// Evlerin ustunde kucuk dugum: takimyildizindakiyle ayni dil (dolu mutlu, halka mutsuz).
	if (!D.HasFamilyNeeds())
	{
		return;
	}
	const float S = UiScale();
	for (int32 I = 0; I < D.Families.Num(); ++I)
	{
		const ASLPlaceable* Home = D.GetHome(I);
		if (!Home || Home->bHeld || !Home->bResidentsArrived)
		{
			continue;
		}
		const FBox Bx = Home->GetComponentsBoundingBox(false);
		const FVector P = Project(FVector(Bx.GetCenter().X, Bx.GetCenter().Y, Bx.Max.Z + 14.f));
		if (P.Z <= 0.f)
		{
			continue;
		}
		if (D.IsFamilyHappy(I))
		{
			FillCircle(P.X, P.Y, 6.f * S, D.Families[I].Tint);
			Arc(P.X, P.Y, 9.f * S, 0.f, 360.f, 1.f, CCream);
		}
		else
		{
			FillCircle(P.X, P.Y, 10.f * S, FLinearColor(CFail.R, CFail.G, CFail.B, 0.25f));
			Arc(P.X, P.Y, 6.f * S, 0.f, 360.f, 2.f, CFail);
		}
	}
}

// =================================================================================================
// Ana menu, jenerik, trailer yazilari (16 Eki). Ayni "gokyuzu pusulasi" dili: panel yok, aralikli harf,
// ince cizgi, soluk krem hap zemin. Arkada canli kasaba doner (ASLLevelDirector::ConfigureMenu).
// =================================================================================================

namespace
{
	const FLinearColor MInk(0.243f, 0.212f, 0.345f);
	const FLinearColor MMuted(0.416f, 0.376f, 0.525f);
	const FLinearColor MLine(0.62f, 0.573f, 0.745f);
	const FLinearColor MCream(0.984f, 0.973f, 0.992f);
}

TArray<FString> ASLHUD::GetCreditsLines() const
{
	// Jenerik = kural kitapcigi 3.4 / 4.2 beyanlarinin oyun ici hali (README ile ayni icerik).
	TArray<FString> L;
	L.Add(TEXT("#SALLANTI"));
	L.Add(TEXT("dengede bir şehir kur"));
	L.Add(TEXT(""));
	L.Add(FString::Printf(TEXT("%s"), *TeamName));
	L.Add(TEXT("CIVIL GAME JAM 2026"));
	L.Add(TEXT(""));
	L.Add(TEXT("#EKİP"));
	for (const FString& M : TeamMembers)
	{
		L.Add(M);
	}
	L.Add(TEXT(""));
	L.Add(TEXT("#HAZIR KAYNAKLAR"));
	L.Add(TEXT("Unreal Engine 5.8 · Epic Games · Unreal Engine EULA"));
	L.Add(TEXT("Motorun temel şekilleri (küp, küre, silindir, koni)"));
	L.Add(TEXT("Epic Games · Unreal Engine EULA"));
	L.Add(TEXT("Cinzel yazı tipi · The Cinzel Project Authors · SIL OFL 1.1"));
	L.Add(TEXT("Jost yazı tipi · The Jost Project Authors · SIL OFL 1.1"));
	L.Add(TEXT("Roboto yazı tipi · Google · Apache 2.0 (motorla gelir)"));
	L.Add(TEXT(""));
	L.Add(TEXT("#YAPAY ZEKÂ KULLANIMI"));
	L.Add(TEXT("Kod: Claude (Anthropic) · oyun kodu, araç betikleri"));
	L.Add(TEXT("Görsel: Claude · binalar, arayüz ve arka plan kodla çizildi"));
	L.Add(TEXT("Ses: Claude'un yazdığı sentez betikleri · efektler ve müzik"));
	L.Add(TEXT("Metin: Claude · diyaloglar ve arayüz metinleri"));
	L.Add(TEXT("Diğer: Claude · tasarım belgesi, test otomasyonu, trailer"));
	L.Add(TEXT(""));
	L.Add(TEXT("#TEŞEKKÜRLER"));
	L.Add(TEXT("KTMMOB İnşaat Mühendisleri Odası"));
	L.Add(TEXT("LootBAG Studios · ODTÜ KALTEV"));
	L.Add(TEXT(""));
	L.Add(TEXT("#Oynadığın için teşekkürler"));
	return L;
}

bool ASLHUD::MenuButton(const FString& Label, float X, float Y, float Scale, FRect& Out)
{
	const FVector2D Sz = Measure(Label, Scale, false);
	Out = { X - 30.f, Y - 6.f, float(Sz.X) + 60.f, float(Sz.Y) + 12.f };
	const bool bH = Hover(Out);
	// Imleç: soldaki elmas dolar, metin kayar, altinda ince cizgi.
	const float Shift = bH ? 8.f : 0.f;
	const float DX = X - 14.f, DY = Y + 0.5f * Sz.Y;
	for (int32 I = 0; I < 10; ++I)
	{
		const float Hw = (I < 5 ? I : 10 - I) * 0.9f;
		if (bH || I == 0 || I == 9 || Hw >= 3.5f)
		{
			DrawRect(bH ? MInk : MLine, DX - Hw, DY - 5.f + I, 2.f * Hw, 1.f);
		}
	}
	TextHalo(Label, X + Shift, Y, MInk, Scale);
	if (bH)
	{
		DrawLine(X + Shift, Y + Sz.Y + 3.f, X + Shift + Sz.X, Y + Sz.Y + 3.f, MInk, 1.f);
	}
	return bH;
}

void ASLHUD::DrawMenu()
{
	const float S = UiScale();
	const float CX = 330.f * S;
	MenuRects.Reset();
	MenuActions.Reset();
	if (MenuPage == ESLMenuPage::Credits)
	{
		DrawCredits();
		return;
	}
	// Baslik: ekranin sol yarisinda, tahta sagda doner.
	TextSpaced(TEXT("SALLANTI"), CX, 150.f * S, 2.3f * S, 14.f * S, MInk);
	const FString Tag = TEXT("dengede bir şehir kur");
	const FVector2D TS = Measure(Tag, 0.9f * S, false);
	TextHalo(Tag, CX - 0.5f * TS.X, 222.f * S, MMuted, 0.9f * S);
	DrawLine(CX - 50.f * S, 262.f * S, CX + 50.f * S, 262.f * S, MLine, 1.f);

	float Y = 320.f * S;
	const float X = CX - 90.f * S, Step = 52.f * S, Sc = 1.05f * S;
	auto Add = [&](const FString& Label, int32 Action)
	{
		FRect R;
		MenuButton(Label, X, Y, Sc, R);
		MenuRects.Add(R);
		MenuActions.Add(Action);
		Y += Step;
	};
	if (MenuPage == ESLMenuPage::Main)
	{
		Add(TEXT("Oyna"), 1);
		Add(TEXT("Bölümler"), 10);
		Add(TEXT("Nasıl oynanır"), 11);
		Add(TEXT("Emeği geçenler"), 12);
		Add(TEXT("Çıkış"), 99);
	}
	else if (MenuPage == ESLMenuPage::Levels)
	{
		Add(TEXT("1 · İlk mahalle"), 1);
		Add(TEXT("2 · Otobüs durağı"), 2);
		Add(TEXT("3 · Komşular"), 3);
		Add(TEXT("‹ Geri"), 20);
	}
	else if (MenuPage == ESLMenuPage::HowTo)
	{
		const TCHAR* Lines[] = {
			TEXT("Sallanan bir adaya şehir kuruyorsun."),
			TEXT("Her bina dengeyi bozar: ağırları merkeze yakın tut."),
			TEXT("Ailelere tıkla, ne istediklerini öğren."),
			TEXT("Seçili aile için ev kur ya da boş bir eve tıkla."),
			TEXT("Herkes mutlu olunca bölüm biter."),
			TEXT(""),
			TEXT("Sol tık: koy / binayı taşı    R, Q: çevir"),
			TEXT("Sağ tık sürükle: kaydır    Tekerlek: yakınlaş"),
			TEXT("Z, X: tahtayı döndür    Esc: bırak    M: müzik"),
		};
		float LY = Y - 10.f * S;
		for (const TCHAR* L : Lines)
		{
			if (*L)
			{
				TextHalo(L, X - 20.f * S, LY, MInk, 0.78f * S);
			}
			LY += 30.f * S;
		}
		Y = LY + 20.f * S;
		Add(TEXT("‹ Geri"), 20);
	}
	const FString Foot = FString::Printf(TEXT("%s  ·  CIVIL GAME JAM 2026"), *TeamName);
	TextHalo(Foot, 40.f * S, Canvas->ClipY - 50.f * S, MMuted, 0.62f * S);
}

void ASLHUD::DrawCredits()
{
	// Yavasca yukari kayan jenerik; tiklama ya da Esc geri doner.
	const float S = UiScale();
	FillRound(0.f, 0.f, Canvas->ClipX, Canvas->ClipY, 0.f, FLinearColor(MCream.R, MCream.G, MCream.B, 0.55f));
	const TArray<FString> Lines = GetCreditsLines();
	const float Speed = 38.f * S;
	const float T = NowReal - CreditsStart;
	float Y = Canvas->ClipY + 20.f * S - T * Speed;
	const float CX = 0.5f * Canvas->ClipX;
	for (const FString& L : Lines)
	{
		if (L.StartsWith(TEXT("#")))
		{
			const FString H = L.Mid(1);
			const bool bTitle = H == TEXT("SALLANTI");
			Y += bTitle ? 0.f : 18.f * S;
			if (Y > -80.f && Y < Canvas->ClipY + 40.f)
			{
				TextSpaced(H, CX, Y, (bTitle ? 2.f : 0.9f) * S, (bTitle ? 12.f : 5.f) * S, MInk);
			}
			Y += (bTitle ? 80.f : 44.f) * S;
		}
		else
		{
			if (!L.IsEmpty() && Y > -40.f && Y < Canvas->ClipY + 40.f)
			{
				const FVector2D Sz = Measure(L, 0.78f * S, false);
				TextHalo(L, CX - 0.5f * Sz.X, Y, MMuted, 0.78f * S);
			}
			Y += 32.f * S;
		}
	}
	if (Y < -40.f)
	{
		CreditsStart = NowReal; // bitince basa sar
	}
	FRect R;
	MenuButton(TEXT("‹ Geri"), 70.f * S, Canvas->ClipY - 70.f * S, 0.9f * S, R);
	MenuRects.Add(R);
	MenuActions.Add(20);
}

void ASLHUD::DoMenuAction(int32 Action)
{
	ASLGameMode* GM = GetWorld()->GetAuthGameMode<ASLGameMode>();
	ASLGameMode::PlaySfx(this, TEXT("SFX_UI_Tik"), 0);
	switch (Action)
	{
	case 1: case 2: case 3:
		if (GM) { GM->TravelToLevel(Action); }
		break;
	case 10: MenuPage = ESLMenuPage::Levels; break;
	case 11: MenuPage = ESLMenuPage::HowTo; break;
	case 12: MenuPage = ESLMenuPage::Credits; CreditsStart = NowReal; break;
	case 20: MenuPage = ESLMenuPage::Main; break;
	case 21: if (GM) { GM->TravelToLevel(0); } break;
	case 22: if (GM) { GM->TravelToLevel(0, true); } break;
	case 99:
		if (APlayerController* PC = GetOwningPlayerController())
		{
			PC->ConsoleCommand(TEXT("quit"));
		}
		break;
	default: break;
	}
}

void ASLHUD::MenuBack()
{
	if (MenuPage != ESLMenuPage::Main)
	{
		MenuPage = ESLMenuPage::Main;
	}
}

void ASLHUD::DrawCinemaOverlay()
{
	// Trailer: ortada baslik karti (gokyuzu zemininde), altta kisa altyazi.
	const float S = UiScale();
	if (!CinemaTitle.IsEmpty())
	{
		// Baslik karti oyun arayuzunun ustunde okunsun: zemin neredeyse opak (0.5'te baslik arayuzde kayboldu).
		FillRound(0.f, 0.f, Canvas->ClipX, Canvas->ClipY, 0.f, FLinearColor(MCream.R, MCream.G, MCream.B, 0.86f * CinemaTitleAlpha));
		const FLinearColor C(MInk.R, MInk.G, MInk.B, CinemaTitleAlpha);
		TextSpaced(CinemaTitle, 0.5f * Canvas->ClipX, 0.42f * Canvas->ClipY, 2.4f * S, 16.f * S, C);
		if (!CinemaSub.IsEmpty())
		{
			const FVector2D Sz = Measure(CinemaSub, 0.95f * S, false);
			Text(CinemaSub, 0.5f * (Canvas->ClipX - Sz.X), 0.42f * Canvas->ClipY + 84.f * S, FLinearColor(MMuted.R, MMuted.G, MMuted.B, CinemaTitleAlpha), 0.95f * S);
		}
	}
	if (!CinemaCaption.IsEmpty())
	{
		const FVector2D Sz = Measure(CinemaCaption, 1.1f * S, false);
		TextHalo(CinemaCaption, 0.5f * (Canvas->ClipX - Sz.X), Canvas->ClipY - 120.f * S, MInk, 1.1f * S);
	}
}
