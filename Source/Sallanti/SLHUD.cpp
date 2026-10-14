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
	const FLinearColor Green(0.35f, 0.85f, 0.45f);
	const FLinearColor Yellow(1.f, 0.78f, 0.2f);
	const FLinearColor Red(1.f, 0.3f, 0.25f);
	const FLinearColor Info(0.85f, 0.92f, 1.f);
	const FLinearColor Panel(0.f, 0.f, 0.f, 0.5f);
	const FLinearColor Dim(1.f, 1.f, 1.f, 0.75f);
	const FLinearColor Paper(0.98f, 0.95f, 0.88f, 0.97f);
	const FLinearColor Ink(0.15f, 0.11f, 0.07f);
	constexpr float HandleW = 34.f;
	constexpr float HandleH = 150.f;
	constexpr float CardH = 92.f;
}

// --- Yardimcilar --------------------------------------------------------------------------------

void ASLHUD::Text(const FString& S, float X, float Y, const FLinearColor& Color, float Scale, bool bLarge)
{
	UFont* Font = GEngine ? (bLarge ? GEngine->GetLargeFont() : GEngine->GetMediumFont()) : nullptr;
	const float Sc = Scale * TextScale;
	if (Color.R + Color.G + Color.B > 1.2f)
	{
		// Acik renkli yaziya golge: acik karton uzerinde okunsun. Koyu yazi (balon, kart) golgesiz.
		DrawText(S, FLinearColor(0.f, 0.f, 0.f, 0.8f), X + 1.5f, Y + 1.5f, Font, Sc);
	}
	DrawText(S, Color, X, Y, Font, Sc);
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
	// 0..FailTilt*1.2 olcegi: yesil (dengede), sari (uyari), kirmizi (devrilme). Sallanma haric egim.
	const float W = BalanceBarWidth, H = 22.f;
	const float X = 0.5f * (Canvas->ClipX - W), Y = 24.f;
	const float Max = FMath::Max(P.FailTilt * 1.2f, 0.1f);
	const float Judged = P.IsCollapsed() ? Max : P.CurrentTilt.Size();
	const float WarnX = W * FMath::Clamp(P.WarnTilt / Max, 0.f, 1.f);
	const float FailX = W * FMath::Clamp(P.FailTilt / Max, 0.f, 1.f);

	DrawRect(Panel, X - 6.f, Y - 6.f, W + 12.f, H + 12.f);
	DrawRect(FLinearColor(Green.R, Green.G, Green.B, 0.25f), X, Y, WarnX, H);
	DrawRect(FLinearColor(Yellow.R, Yellow.G, Yellow.B, 0.25f), X + WarnX, Y, FailX - WarnX, H);
	DrawRect(FLinearColor(Red.R, Red.G, Red.B, 0.25f), X + FailX, Y, W - FailX, H);

	const FLinearColor Fill = Judged >= P.FailTilt ? Red : Judged >= P.WarnTilt ? Yellow : Green;
	DrawRect(Fill, X, Y + 4.f, W * FMath::Clamp(Judged / Max, 0.f, 1.f), H - 8.f);
	DrawLine(X + WarnX, Y - 3.f, X + WarnX, Y + H + 3.f, Yellow, 2.f);
	DrawLine(X + FailX, Y - 3.f, X + FailX, Y + H + 3.f, Red, 2.f);

	// Tahmin: elindekini buraya koyarsan bar nereye gelir. Dikey cizgi + ustunde kucuk kare.
	const float Pred = PredictPlacement(P, B);
	if (Pred >= 0.f)
	{
		const float PX = X + W * FMath::Clamp(Pred / Max, 0.f, 1.f);
		const FLinearColor PCol = Pred >= P.FailTilt ? Red : Pred >= P.WarnTilt ? Yellow : FLinearColor::White;
		DrawLine(PX, Y - 8.f, PX, Y + H + 8.f, PCol, 3.f);
		DrawRect(PCol, PX - 5.f, Y - 14.f, 10.f, 6.f);
	}

	if (P.BalanceState == ESLBalanceState::Failing)
	{
		// Devrilme sayaci: barin altinda azalan serit.
		const float Left = FMath::Clamp(1.f - P.FailTimer / FMath::Max(P.FailSeconds, 0.01f), 0.f, 1.f);
		DrawRect(Red, X, Y + H + 6.f, W * Left, 5.f);
	}
	const FString Label = TEXT("DENGE");
	const FVector2D Sz = Measure(Label, 0.8f, false);
	Text(Label, X - Sz.X - 16.f, Y + 0.5f * (H - Sz.Y), Dim, 0.8f);
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
	const float Scale = 0.85f;
	const TArray<FString> Lines = Wrap(S, 640.f, Scale);
	const float LineH = Measure(TEXT("Ag"), Scale, false).Y + 2.f;
	float W = 0.f;
	for (const FString& L : Lines)
	{
		W = FMath::Max(W, Measure(L, Scale, false).X);
	}
	const float X = 0.5f * (Canvas->ClipX - W), Y = 68.f;
	DrawRect(Panel, X - 14.f, Y - 8.f, W + 28.f, LineH * Lines.Num() + 16.f);
	DrawRect(C, X - 14.f, Y - 8.f, 4.f, LineH * Lines.Num() + 16.f); // renk seridi: kirmizi / sari / bilgi
	for (int32 I = 0; I < Lines.Num(); ++I)
	{
		Text(Lines[I], X, Y + I * LineH, C, Scale);
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
	const float Pad = 12.f, TailH = 18.f;
	const float BW = W + 2.f * Pad, BH = LineH * Lines.Num() + 2.f * Pad;
	// Balon capanin ustunde; ekrandan tasmasin.
	const float BX = FMath::Clamp(Screen.X - 0.5f * BW, 12.f, Canvas->ClipX - BW - 12.f);
	const float BY = FMath::Clamp(Screen.Y - BH - TailH, 120.f, Canvas->ClipY - BH - 12.f);
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.35f), BX + 3.f, BY + 4.f, BW, BH); // golge
	DrawRect(Paper, BX, BY, BW, BH);
	// Kuyruk: capaya dogru daralan seritler.
	const float TX = FMath::Clamp(Screen.X, BX + 16.f, BX + BW - 16.f);
	for (int32 I = 0; I < 6; ++I)
	{
		const float Wd = 18.f * (1.f - I / 6.f);
		DrawRect(Paper, TX - 0.5f * Wd, BY + BH + I * (TailH / 6.f), Wd, TailH / 6.f + 0.5f);
	}
	for (int32 I = 0; I < Lines.Num(); ++I)
	{
		Text(Lines[I], BX + Pad, BY + Pad + I * LineH, Ink, Scale);
	}
}

void ASLHUD::DrawTasks(const ASLLevelDirector& D)
{
	// Sol ust: sag taraf cekmecenin.
	float W = Measure(D.LevelTitle, 0.95f, false).X;
	for (const FSLTask& T : D.Tasks)
	{
		W = FMath::Max(W, Measure(TEXT("[x]  ") + T.Text + TEXT("  0/0"), 0.8f, false).X);
	}
	const float X = 24.f;
	float Y = 70.f;
	const float LineH = 22.f * TextScale;
	DrawRect(Panel, X - 10.f, Y - 8.f, W + 20.f, LineH * (D.Tasks.Num() + 1.4f) + 10.f);
	Text(D.LevelTitle, X, Y, FLinearColor(1.f, 0.92f, 0.75f), 0.95f);
	Y += LineH * 1.3f;
	for (int32 I = 0; I < D.Tasks.Num(); ++I)
	{
		const FSLTask& T = D.Tasks[I];
		const bool bDone = D.IsTaskDone(I);
		const bool bCurrent = I == D.CurrentTask && !D.bLevelComplete;
		FString S = FString::Printf(TEXT("%s  %s"), bDone ? TEXT("[x]") : TEXT("[  ]"), *T.Text);
		if (bCurrent && D.ProgressGoal > 1)
		{
			S += FString::Printf(TEXT("  %d/%d"), D.Progress, D.ProgressGoal);
		}
		const FLinearColor C = bDone ? FLinearColor(0.6f, 0.9f, 0.6f, 0.8f) : bCurrent ? FLinearColor::White : FLinearColor(1.f, 1.f, 1.f, 0.4f);
		Text(S, X, Y, C, 0.8f);
		Y += LineH;
	}
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
	const float PanelH = 46.f + N * (CardH + 8.f);
	const float PX = Canvas->ClipX - DrawerWidth * DrawerOpen;
	const float PY = 0.5f * (Canvas->ClipY - PanelH);
	DrawerRect = { PX, PY, DrawerWidth, PanelH };
	HandleRect = { PX - HandleW, 0.5f * (Canvas->ClipY - HandleH), HandleW, HandleH };

	// Tutamak: cekmecenin kulpu. Uzerinde harf harf "BINALAR".
	const FLinearColor HandleC(DrawerColor.R * 0.8f, DrawerColor.G * 0.8f, DrawerColor.B * 0.8f, 0.97f);
	DrawRect(HandleC, HandleRect.X, HandleRect.Y, HandleRect.W, HandleRect.H);
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.35f), HandleRect.X + 4.f, HandleRect.Y + 10.f, 3.f, HandleRect.H - 20.f);
	const FString Word = TEXT("BİNALAR");
	const float LetterH = HandleH / (Word.Len() + 1);
	for (int32 I = 0; I < Word.Len(); ++I)
	{
		const FString Ch = Word.Mid(I, 1);
		const FVector2D Sz = Measure(Ch, 0.7f, false);
		Text(Ch, HandleRect.X + 0.5f * (HandleW - Sz.X) + 2.f, HandleRect.Y + LetterH * (I + 0.5f), FLinearColor::White, 0.7f);
	}

	CardRects.Reset();
	if (DrawerOpen <= 0.01f)
	{
		return;
	}
	DrawRect(DrawerColor, PX, PY, DrawerWidth, PanelH);
	DrawRect(FLinearColor(0.85f, 0.75f, 0.5f, 0.6f), PX + 20.f, PY - 6.f, 70.f, 14.f); // koli bandi
	Text(TEXT("Bina çekmecesi"), PX + 14.f, PY + 12.f, FLinearColor::White, 0.85f);
	float Y = PY + 46.f;
	for (int32 I = 0; I < N; ++I, Y += CardH + 8.f)
	{
		const FSLSlot& S = B.Slots[I];
		const bool bOpen = B.IsSlotUnlocked(I);
		const bool bSel = B.SelectedSlot == I && !B.IsMoving();
		const FRect R{ PX + 10.f, Y, DrawerWidth - 20.f, CardH };
		CardRects.Add(R);
		DrawRect(bSel ? FLinearColor(0.2f, 0.5f, 0.25f, 0.95f) : FLinearColor(0.96f, 0.92f, 0.82f, bOpen ? 0.95f : 0.45f), R.X, R.Y, R.W, R.H);

		// Kutu cizimi: on yuz + yan yuz + ust; boyutla olcekli, binanin karton renginde, kapisiyla.
		const float Unit = 13.f;
		const float BW = S.SizeCells.X * Unit, BHt = (S.bFlat ? 0.3f : S.SizeCells.Z) * Unit, BD = S.SizeCells.Y * Unit * 0.27f;
		const float IX = R.X + 12.f, IY = R.Y + R.H - 14.f - BHt;
		const FLinearColor Body = bOpen ? S.Tint : FLinearColor(0.5f, 0.5f, 0.5f);
		DrawRect(Body * 1.1f, IX + BD, IY - BD, BW, BD);  // ust
		DrawRect(Body * 0.7f, IX + BW, IY - BD, BD, BHt); // yan
		DrawRect(Body, IX, IY, BW, BHt);                  // on
		if (S.bHasEntrance)
		{
			const float DW = FMath::Max(4.f, BW * 0.25f), DH = FMath::Max(6.f, BHt * 0.5f);
			DrawRect(FLinearColor(0.18f, 0.11f, 0.07f), IX + 0.5f * (BW - DW), IY + BHt - DH, DW, DH); // kapi
		}

		const FLinearColor TextC = bSel ? FLinearColor::White : FLinearColor(0.15f, 0.11f, 0.07f, bOpen ? 1.f : 0.5f);
		const float TX = R.X + 100.f;
		Text(FString::Printf(TEXT("%d  %s"), I + 1, *S.Label), TX, R.Y + 10.f, TextC, 0.9f);
		if (bOpen)
		{
			Text(S.Mass < 1.f ? FString::Printf(TEXT("kütle %.1f"), S.Mass) : FString::Printf(TEXT("kütle %.0f"), S.Mass), TX, R.Y + 38.f, TextC, 0.7f);
			Text(S.bFlat ? FString(TEXT("düz, 1x1")) : FString::Printf(TEXT("%dx%dx%d"), S.SizeCells.X, S.SizeCells.Y, S.SizeCells.Z), TX, R.Y + 60.f, TextC, 0.7f);
		}
		else
		{
			Text(TEXT("kilitli"), TX, R.Y + 40.f, TextC, 0.75f);
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
		C = Green;
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
	const float Scale = 1.6f;
	const FVector2D Sz = Measure(S, Scale, true);
	const float X = 0.5f * (Canvas->ClipX - Sz.X), Y = Canvas->ClipY - 70.f - Sz.Y;
	DrawRect(Panel, X - 24.f, Y - 10.f, Sz.X + 48.f, Sz.Y + 20.f);
	Text(S, X, Y, C, Scale, true);
}

void ASLHUD::DrawTestPanel(const ASLPlatform& P, const USLBuildComponent& B)
{
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
	Line(X, Y, TEXT("Backspace geri al  F5 baştan  G ızgara  Tab çekmece  ~ konsol: SLKnobs, SLSet"), Dim);
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
				return true;
			}
			B->SelectSlot(I);
			if (bCloseDrawerOnSelect)
			{
				bDrawerWanted = false; // platform gorunsun
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
	bDrawerWanted = bDragMoved ? DrawerOpen > 0.4f : !bDrawerWanted;
}
