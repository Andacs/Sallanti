#include "SLHUD.h"

#include "SLBuildComponent.h"
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
	const FLinearColor Panel(0.f, 0.f, 0.f, 0.5f);
	const FLinearColor Dim(1.f, 1.f, 1.f, 0.75f);
}

void ASLHUD::Text(const FString& S, float X, float Y, const FLinearColor& Color, float Scale, bool bLarge)
{
	UFont* Font = GEngine ? (bLarge ? GEngine->GetLargeFont() : GEngine->GetMediumFont()) : nullptr;
	const float Sc = Scale * TextScale;
	// Golge: acik karton uzerinde okunsun.
	DrawText(S, FLinearColor(0.f, 0.f, 0.f, 0.8f), X + 1.5f, Y + 1.5f, Font, Sc);
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

void ASLHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas)
	{
		return;
	}
	const ASLPlayerController* PC = Cast<ASLPlayerController>(GetOwningPlayerController());
	ASLGameMode* GM = GetWorld()->GetAuthGameMode<ASLGameMode>();
	const ASLPlatform* P = GM ? GM->GetPlatform() : nullptr;
	const ASLLevelDirector* D = GM ? GM->GetDirector() : nullptr;
	const USLBuildComponent* B = PC ? PC->Build.Get() : nullptr;
	if (!P || !B)
	{
		return;
	}

	DrawBalanceBar(*P);
	if (D)
	{
		DrawTasks(*D);
	}
	DrawSlotBar(*B);
	DrawCenterMessage(*P, GM->IsLevelComplete(), D ? D->LevelTitle : FString());
	if (bShowTestPanel)
	{
		DrawTestPanel(*P, *B);
	}
}

void ASLHUD::DrawBalanceBar(const ASLPlatform& P)
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

void ASLHUD::DrawTasks(const ASLLevelDirector& D)
{
	// Genislik metne gore: platformun arka kosesini gereksiz kapatmasin.
	float W = Measure(D.LevelTitle, 1.f, false).X;
	for (const FSLTask& T : D.Tasks)
	{
		W = FMath::Max(W, Measure(TEXT("[x]  ") + T.Text + TEXT("  0/0"), 0.85f, false).X);
		W = FMath::Max(W, Measure(T.Hint, 0.75f, false).X);
	}
	const float X = Canvas->ClipX - W - 24.f;
	float Y = 80.f;
	const float LineH = 24.f * TextScale;
	const int32 Shown = D.Tasks.Num();
	DrawRect(Panel, X - 12.f, Y - 10.f, W + 24.f, LineH * (Shown + 3.2f));
	Text(D.LevelTitle, X, Y, FLinearColor(1.f, 0.92f, 0.75f), 1.f);
	Y += LineH * 1.4f;
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
		Text(S, X, Y, C, 0.85f);
		Y += LineH;
	}
	if (D.Tasks.IsValidIndex(D.CurrentTask) && !D.bLevelComplete && !D.Tasks[D.CurrentTask].Hint.IsEmpty())
	{
		Y += 4.f;
		Text(D.Tasks[D.CurrentTask].Hint, X, Y, FLinearColor(1.f, 0.85f, 0.55f), 0.75f);
	}
}

void ASLHUD::DrawSlotBar(const USLBuildComponent& B)
{
	const int32 N = B.Slots.Num();
	const float BoxW = 150.f, BoxH = 58.f, Gap = 10.f;
	const float Total = N * BoxW + (N - 1) * Gap;
	float X = 0.5f * (Canvas->ClipX - Total);
	const float Y = Canvas->ClipY - BoxH - 44.f;
	for (int32 I = 0; I < N; ++I, X += BoxW + Gap)
	{
		const FSLSlot& S = B.Slots[I];
		const bool bOpen = B.IsSlotUnlocked(I);
		const bool bSel = B.SelectedSlot == I;
		DrawRect(bSel ? FLinearColor(0.15f, 0.45f, 0.2f, 0.8f) : Panel, X, Y, BoxW, BoxH);
		// Slotun rengi: karttaki kucuk kare = nesnenin karton rengi.
		DrawRect(bOpen ? S.Tint : FLinearColor(0.3f, 0.3f, 0.3f), X + 8.f, Y + 8.f, 14.f, 14.f);
		Text(FString::Printf(TEXT("%d  %s"), I + 1, *S.Label), X + 30.f, Y + 5.f, bOpen ? FLinearColor::White : FLinearColor(1.f, 1.f, 1.f, 0.35f), 0.85f);
		Text(bOpen ? (S.Mass < 1.f ? FString::Printf(TEXT("kütle %.1f"), S.Mass) : FString::Printf(TEXT("kütle %.0f"), S.Mass)) : FString(TEXT("kilitli")), X + 30.f, Y + 31.f,
			bOpen ? Dim : FLinearColor(1.f, 1.f, 1.f, 0.3f), 0.7f);
	}
	FString Hint;
	if (B.IsMoving())
	{
		Hint = FString::Printf(TEXT("Elde: %s   Sol tık: buraya koy   R: döndür   Sağ tık: eski yerine"), *B.GetMoving()->TypeId.ToString());
	}
	else if (B.State != ESLBuildState::Idle)
	{
		Hint = TEXT("Sol tık: koy   R: döndür   Sağ tık: bırak");
	}
	else
	{
		Hint = TEXT("1-4: nesne seç   Bir binaya tıkla: taşı");
	}
	const FVector2D Sz = Measure(Hint, 0.8f, false);
	Text(Hint, 0.5f * (Canvas->ClipX - Sz.X), Canvas->ClipY - 34.f, Dim, 0.8f);
}

void ASLHUD::DrawCenterMessage(const ASLPlatform& P, bool bLevelComplete, const FString& LevelTitle)
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
	// Gorev paneliyle (sag ust) cakismasin: slot cubugunun ustunde.
	const float X = 0.5f * (Canvas->ClipX - Sz.X), Y = Canvas->ClipY - 190.f - Sz.Y;
	DrawRect(Panel, X - 24.f, Y - 10.f, Sz.X + 48.f, Sz.Y + 20.f);
	Text(S, X, Y, C, Scale, true);
}

void ASLHUD::DrawTestPanel(const ASLPlatform& P, const USLBuildComponent& B)
{
	const float X = 24.f;
	float Y = 80.f;
	DrawRect(Panel, X - 10.f, Y - 8.f, 560.f, 22.f * TextScale * 7.f + 16.f);
	Line(X, Y, TEXT("TEST PANELİ (H)"), FLinearColor(1.f, 0.9f, 0.5f));
	if (B.State != ESLBuildState::Idle)
	{
		const bool bOk = B.State == ESLBuildState::Placing;
		Line(X, Y, FString::Printf(TEXT("Hücre (%d, %d)  seviye %d  %s%s"), B.GetGhostCellMin().X, B.GetGhostCellMin().Y,
			B.GetGhostLevel(), bOk ? TEXT("geçerli") : TEXT("GEÇERSİZ: "), bOk ? TEXT("") : B.GetBlockedReason()),
			bOk ? Green : Red);
	}
	else
	{
		Line(X, Y, TEXT("Elde bir şey yok"), Dim);
	}
	Line(X, Y, FString::Printf(TEXT("Eğim %.1f°  hedef %.1f°  sallanma dahil %.1f°"), P.CurrentTilt.Size(), P.TargetTiltDegrees, P.CurrentTiltDegrees));
	Line(X, Y, FString::Printf(TEXT("CoM (%.0f, %.0f) cm  |%.0f|  toplam kütle %.1f  nesne %d"),
		P.CenterOfMass.X, P.CenterOfMass.Y, P.CenterOfMass.Size(), P.TotalMass, B.PlacedCount));
	Line(X, Y, FString::Printf(TEXT("PlatformMass %.0f  TiltPerCm %.3f  uyarı %.0f°  devrilme %.0f° / %.1f sn"),
		P.PlatformMass, P.TiltPerCm, P.WarnTilt, P.FailTilt, P.FailSeconds), Dim);
	Line(X, Y, TEXT("Backspace geri al  F5 baştan  G ızgara  ~ konsol: SLKnobs, SLSet <ad> <değer>"), Dim);
	Line(X, Y, TEXT("SLTestLevel1: bölümü otomatik oynar ve ölçer"), Dim);
}
