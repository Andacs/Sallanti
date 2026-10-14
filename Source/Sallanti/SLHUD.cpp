#include "SLHUD.h"

#include "SLBuildComponent.h"
#include "SLCameraPawn.h"
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

	const ASLPlayerController* PC = Cast<ASLPlayerController>(GetOwningPlayerController());
	ASLGameMode* GM = GetWorld()->GetAuthGameMode<ASLGameMode>();
	const ASLPlatform* P = GM ? GM->GetPlatform() : nullptr;
	const ASLLevelDirector* D = GM ? GM->GetDirector() : nullptr;
	const USLBuildComponent* B = PC ? PC->Build.Get() : nullptr;
	if (!P || !B)
	{
		return;
	}

	if (D)
	{
		DrawSpeech(*D); // dunyaya bagli: once, panellerin altinda kalsin
		DrawTasks(*D);
	}
	DrawBalanceBar(*P, *B);
	DrawHint(*P, *B, D);
	DrawDrawer(*B, Dt);
	DrawCenterMessage(*P, GM->IsLevelComplete());
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
	// Sol ust kart: konum rozeti + bolum adi, altinda yuvarlak onay kutulu gorevler. Sag taraf cekmecenin.
	const float TitleScale = 1.0f, ItemScale = 0.8f;
	float W = Measure(D.LevelTitle, TitleScale, false).X + 40.f;
	for (const FSLTask& T : D.Tasks)
	{
		W = FMath::Max(W, Measure(T.Text + TEXT("  0/0"), ItemScale, false).X + 34.f);
	}
	const float LineH = 26.f * TextScale;
	const float X = 20.f, Y0 = 16.f;
	const float H = LineH * (D.Tasks.Num() + 1.5f) + 18.f;
	Card(X, Y0, W + 36.f, H, 18.f, PanelColor);

	// Konum rozeti: mercan daire + beyaz ic nokta + asagi ucgen.
	const float PinX = X + 26.f, PinY = Y0 + 26.f;
	FillTri(PinX, PinY + 15.f, 13.f, -12.f, Red); // ters ucgen: tepesi asagida
	FillCircle(PinX, PinY, 8.f, Red);
	FillCircle(PinX, PinY, 3.f, FLinearColor::White);
	const FVector2D TSz = Measure(D.LevelTitle, TitleScale, false);
	Text(D.LevelTitle, X + 44.f, PinY - 0.5f * TSz.Y, Ink, TitleScale);

	float Y = Y0 + 18.f + LineH * 1.3f;
	for (int32 I = 0; I < D.Tasks.Num(); ++I)
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
	if (S.Id == TEXT("Ev"))
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

	const int32 N = B.Slots.Num();
	const float HeaderH = 58.f;
	const float PanelH = HeaderH + N * (CardH + 10.f) + 6.f;
	const float Margin = 16.f;
	const float PX = Canvas->ClipX - (DrawerWidth + Margin) * DrawerOpen;
	const float PY = FMath::Max(16.f, 0.5f * (Canvas->ClipY - PanelH));
	DrawerRect = { PX, PY, DrawerWidth, PanelH };
	HandleRect = { PX - HandleW, 0.5f * (Canvas->ClipY - HandleH), HandleW + 14.f, HandleH };

	// Tutamak: panelin solunda yuvarlak sekme; ev simgesi, harf harf "BINALAR", yon oku.
	Card(HandleRect.X, HandleRect.Y, HandleRect.W, HandleRect.H, 14.f, PanelColor);
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
		const float AX = HandleRect.X + 0.5f * HandleW, AY = HandleRect.Y + HandleH - 14.f;
		const float D = bDrawerWanted ? 3.f : -3.f;
		DrawLine(AX - D, AY - 5.f, AX + D, AY, Muted, 2.f);
		DrawLine(AX + D, AY, AX - D, AY + 5.f, Muted, 2.f);
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
		const float AX = PX + DrawerWidth - 28.f, AY = PY + 30.f;
		DrawLine(AX - 3.f, AY - 6.f, AX + 3.f, AY, Muted, 2.5f);
		DrawLine(AX + 3.f, AY, AX - 3.f, AY + 6.f, Muted, 2.5f);
	}
	float Y = PY + HeaderH;
	for (int32 I = 0; I < N; ++I, Y += CardH + 10.f)
	{
		const FSLSlot& S = B.Slots[I];
		const bool bOpen = B.IsSlotUnlocked(I);
		const bool bSel = B.SelectedSlot == I && !B.IsMoving();
		const FRect R{ PX + 12.f, Y, DrawerWidth - 24.f, CardH };
		CardRects.Add(R);
		if (bSel)
		{
			FillRound(R.X - 2.f, R.Y - 2.f, R.W + 4.f, R.H + 4.f, 16.f, Teal); // secili: turkuaz cerceve
		}
		FillRound(R.X, R.Y, R.W, R.H, 14.f, bOpen ? FLinearColor::White : FLinearColor(0.95f, 0.94f, 0.93f));

		const float Thumb = CardH - 16.f;
		DrawSlotThumb(S, R.X + 8.f, R.Y + 8.f, Thumb, bOpen);

		const float TX = R.X + Thumb + 20.f;
		const FLinearColor TextC = bOpen ? Ink : Muted;
		Text(S.Label, TX, R.Y + 10.f, TextC, 0.9f);
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
			FillRound(BX, BY, BtnW, BtnH, 0.5f * BtnH, Teal);
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
		S = TEXT("BÖLÜM TAMAMLANDI   Space: tekrar");
		C = Teal;
	}
	else
	{
		switch (P.BalanceState)
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
	Card(X - 54.f, Y - 12.f, Sz.X + 84.f, BH, 0.5f * BH, PanelColor);
	FillCircle(X - 28.f, Y - 12.f + 0.5f * BH, 9.f, C);
	Text(S, X, Y, C == Yellow ? FLinearColor(0.75f, 0.52f, 0.05f) : C, Scale, true);
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

bool ASLHUD::HandleMouseDown(const FVector2D& Mouse)
{
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
		for (int32 I = 0; I < CardRects.Num(); ++I)
		{
			if (!CardRects[I].Contains(Mouse))
			{
				continue;
			}
			ASLPlayerController* PC = Cast<ASLPlayerController>(GetOwningPlayerController());
			USLBuildComponent* B = PC ? PC->Build.Get() : nullptr;
			if (!B)
			{
				return true;
			}
			if (!B->IsSlotUnlocked(I))
			{
				Flash(TEXT("Bu bina henüz kilitli: ailenin istediği görevi bitirince açılır."));
				ASLGameMode::PlaySfx(this, TEXT("SFX_Gecersiz"), 0, 0.8f);
				return true;
			}
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
