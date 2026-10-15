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

void ASLHUD::Text(const FString& S, float X, float Y, const FLinearColor& Color, float Scale, bool bLarge)
{
	UFont* Font = GEngine ? (bLarge ? GEngine->GetLargeFont() : GEngine->GetMediumFont()) : nullptr;
	const float Sc = Scale * TextScale;
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
	UFont* Font = GEngine ? (bLarge ? GEngine->GetLargeFont() : GEngine->GetMediumFont()) : nullptr;
	float W = 0.f, H = 0.f;
	GetTextSize(S, W, H, Font, Scale * TextScale);
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

	if (D)
	{
		DrawHomeFaces(*D);
		DrawTasks(*D);
		DrawFamilies(*D);
		DrawFamilyOverlays(*GM->GetDirector());
		DrawSpeech(*D); // sol kartlarin ustunde: balon kenara itilince kartin altinda kaliyordu
	}
	DrawBalanceBar(*P, *B);
	DrawHint(*P, *B, D);
	DrawDrawer(*B, Dt);
	if (D)
	{
		DrawClock(*D);
	}
	DrawCenterMessage(*P, GM->IsLevelComplete());
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
			Out = FString::Printf(TEXT("%s seçili: çekmeceden Ev kur ya da boş bir eve tıkla."), *D->Families[Sel].Name);
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
				? FString::Printf(TEXT("Çekmeceden %s kartına tıkla."), *Label)
				: FString::Printf(TEXT("Sağdaki çekmeceyi çek ve %s seç (kısayol: %d)."), *Label, Slot + 1);
			return;
		}
		case ESLTaskKind::FamilyMovedIn:
			Out = TEXT("Aile kapıya yürüyor. Aile ağırlık yapmaz, platformu izle.");
			return;
		case ESLTaskKind::ConnectRoads:
			Out = FString::Printf(TEXT("Çekmeceden Yol seç. Bağlı bina: %d / %d. Kapısı yanlış yöne bakan binayı tıklayıp R / Q ile çevir."),
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
	Card(BX, BY, BW, BH, 16.f, FLinearColor(1.f, 1.f, 1.f, 0.98f));
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
	Card(BX, BY, BW, BH, 12.f, Fill);
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
				const float R = Hov == I ? 15.f : 12.f;
				FillCircle(S.X + 1.f, S.Y + Bob + 3.f, R, FLinearColor(0.30f, 0.20f, 0.40f, 0.18f));
				FillCircle(S.X, S.Y + Bob, R, Yellow);
				const FVector2D Q = Measure(TEXT("?"), 0.8f, false);
				Text(TEXT("?"), S.X - 0.5f * Q.X, S.Y + Bob - 0.5f * Q.Y, FLinearColor::White, 0.8f);
			}
		}
	}
	if (D.GetSelectedFamily() != INDEX_NONE)
	{
		DrawWishCard(D, D.GetSelectedFamily());
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
	if (MusicRect.W > 0.f && MusicRect.Contains(Mouse) && GM)
	{
		GM->ToggleMusic();
		ASLGameMode::PlaySfx(this, TEXT("SFX_UI_Tik"), 0);
		return true;
	}
	if (GM && GM->IsRoundOver())
	{
		const ASLLevelDirector* Dir = GM->GetDirector();
		if (NextRect.W > 0.f && NextRect.Contains(Mouse) && Dir && Dir->NextLevelNumber > 0)
		{
			GM->TravelToLevel(Dir->NextLevelNumber);
			return true;
		}
		if (AgainRect.W > 0.f && AgainRect.Contains(Mouse))
		{
			GM->TravelToLevel(GM->LevelNumber);
			return true;
		}
	}
	if (DrawerOpen > 0.5f && HeaderCloseRect.Contains(Mouse))
	{
		SetDrawerOpen(false);
		return true;
	}
	if (ASLLevelDirector* Dir = GM ? GM->GetDirector() : nullptr)
	{
		const int32 Fam = FamilyUnderMouse(*Dir);
		if (Fam != INDEX_NONE)
		{
			Dir->SelectFamily(Fam);
			return true;
		}
	}
	if (HandleRect.Contains(Mouse))
	{
		bDragging = true;
		bDragMoved = false;
		DragStartMouseX = Mouse.X;
		DragStartOpen = DrawerOpen;
		return true;
	}
	if (DrawerOpen > 0.01f && DrawerRect.Contains(Mouse))
	{
		for (int32 K = 0; K < CardRects.Num(); ++K)
		{
			if (!CardRects[K].Contains(Mouse) || !CardSlots.IsValidIndex(K))
			{
				continue;
			}
			const int32 I = CardSlots[K];
			ASLPlayerController* PC = Cast<ASLPlayerController>(GetOwningPlayerController());
			USLBuildComponent* B = PC ? PC->Build.Get() : nullptr;
			if (!B)
			{
				return true;
			}
			if (!B->IsSlotUnlocked(I))
			{
				ShakeCard = K;
				ShakeAt = NowReal;
				Flash(TEXT("Bu bina henüz kilitli: ailenin istediği görevi bitirince açılır."));
				ASLGameMode::PlaySfx(this, TEXT("SFX_Gecersiz"), 0, 0.8f);
				return true;
			}
			PressedCard = K;
			PressedAt = NowReal;
			B->SelectSlot(I);
			ASLGameMode::PlaySfx(this, TEXT("SFX_UI_Tik"), 0);
			if (bCloseDrawerOnSelect)
			{
				bDrawerWanted = false; // platform gorunsun (kapanis sesi yok: bir olay, bir ses)
			}
			return true;
		}
		return true; // cekmecenin bos yerine tiklama: oyuna gecmesin
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
