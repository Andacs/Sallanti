#include "SLTestHUD.h"

#include "SLBuildComponent.h"
#include "SLGameMode.h"
#include "SLPlatform.h"
#include "SLPlayerController.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"

void ASLTestHUD::Line(float X, float& Y, const FString& Text, const FLinearColor& Color)
{
	UFont* Font = GEngine ? GEngine->GetMediumFont() : nullptr;
	// Golge: acik karton uzerinde okunsun.
	DrawText(Text, FLinearColor(0.f, 0.f, 0.f, 0.8f), X + 1.5f, Y + 1.5f, Font, TextScale);
	DrawText(Text, Color, X, Y, Font, TextScale);
	Y += 22.f * TextScale;
}

void ASLTestHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas)
	{
		return;
	}

	const ASLPlayerController* PC = Cast<ASLPlayerController>(GetOwningPlayerController());
	const ASLGameMode* GM = GetWorld()->GetAuthGameMode<ASLGameMode>();
	const ASLPlatform* P = GM ? GM->GetPlatform() : nullptr;
	const USLBuildComponent* B = PC ? PC->Build.Get() : nullptr;
	const float X = 24.f;
	float Y = 20.f;

	if (!bShowTestPanel)
	{
		Line(X, Y, TEXT("H: test paneli"), FLinearColor(1.f, 1.f, 1.f, 0.6f));
		return;
	}
	if (!P || !B)
	{
		Line(X, Y, TEXT("Platform ya da BuildComponent yok"), FLinearColor::Red);
		return;
	}

	// --- Secim ---
	if (B->Slots.IsValidIndex(B->SelectedSlot))
	{
		const FSLSlot& S = B->Slots[B->SelectedSlot];
		Line(X, Y, FString::Printf(TEXT("Slot %d: %s   %dx%dx%d modül (%dx%dx%d cm)   kütle %.1f"),
			B->SelectedSlot + 1, *S.Label, S.SizeCells.X, S.SizeCells.Y, S.SizeCells.Z,
			FMath::RoundToInt(S.SizeCells.X * P->GridSize), FMath::RoundToInt(S.SizeCells.Y * P->GridSize),
			FMath::RoundToInt(S.SizeCells.Z * P->GridSize), S.Mass));
		const bool bOk = B->State == ESLBuildState::Placing;
		Line(X, Y, FString::Printf(TEXT("Hücre (%d, %d)   seviye %d   %s"),
			B->GetGhostCellMin().X, B->GetGhostCellMin().Y, B->GetGhostLevel(),
			bOk ? TEXT("YERLEŞTİRİLEBİLİR") : TEXT("GEÇERSİZ: kenardan taşıyor ya da dışarıda")),
			bOk ? FLinearColor(0.4f, 1.f, 0.5f) : FLinearColor(1.f, 0.4f, 0.35f));
	}
	else
	{
		Line(X, Y, TEXT("Slot seçilmedi: 1 Kutu  2 Orta nesne  3 Küçük bina  4 Orta bina"), FLinearColor(1.f, 0.9f, 0.5f));
		Y += 22.f * TextScale;
	}
	Y += 8.f;

	// --- Denge ---
	Line(X, Y, FString::Printf(TEXT("Eğim %.1f°  (hedef %.1f°, sallanma dahil %.1f°)   nesne %d   toplam kütle %.1f"),
		P->CurrentTilt.Size(), P->TargetTiltDegrees, P->CurrentTiltDegrees, B->PlacedCount, P->TotalMass));
	Line(X, Y, FString::Printf(TEXT("Ağırlık merkezi (%.0f, %.0f) cm   merkeze %.0f cm   PlatformMass %.0f   TiltPerCm %.3f"),
		P->CenterOfMass.X, P->CenterOfMass.Y, P->CenterOfMass.Size(), P->PlatformMass, P->TiltPerCm));

	if (P->ReleasedCount > 0)
	{
		Line(X, Y, FString::Printf(TEXT("Fiziğe bırakıldı %d   platformda kayan %d   düşen %d"),
			P->ReleasedCount, P->ReleasedOnDeck, P->FallenCount), FLinearColor(1.f, 0.6f, 0.3f));
	}

	// Egim cubugu: 0..MaxTilt. ReleaseTilt (sari) ve FailTilt (kirmizi) cizgileri.
	const float BarW = 420.f, BarH = 16.f;
	const float BarY = Y + 4.f;
	const float Max = FMath::Max(P->MaxTilt, 0.1f);
	const float Frac = FMath::Clamp(P->CurrentTiltDegrees / Max, 0.f, 1.f);
	const FLinearColor Fill = P->CurrentTiltDegrees >= P->FailTilt ? FLinearColor(0.95f, 0.2f, 0.15f)
		: P->CurrentTiltDegrees >= P->ReleaseTilt ? FLinearColor(1.f, 0.75f, 0.1f) : FLinearColor(0.3f, 0.85f, 0.4f);
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.55f), X, BarY, BarW, BarH);
	DrawRect(Fill, X, BarY, BarW * Frac, BarH);
	const float RelX = X + BarW * FMath::Clamp(P->ReleaseTilt / Max, 0.f, 1.f);
	const float FailX = X + BarW * FMath::Clamp(P->FailTilt / Max, 0.f, 1.f);
	DrawLine(RelX, BarY - 4.f, RelX, BarY + BarH + 4.f, FLinearColor(1.f, 0.85f, 0.2f), 2.f);
	DrawLine(FailX, BarY - 4.f, FailX, BarY + BarH + 4.f, FLinearColor(1.f, 0.2f, 0.15f), 2.f);
	Y = BarY + BarH + 6.f;
	Line(X, Y, FString::Printf(TEXT("sarı çizgi = ReleaseTilt %.0f°   kırmızı = FailTilt %.0f°   çubuğun ucu = MaxTilt %.0f°"),
		P->ReleaseTilt, P->FailTilt, P->MaxTilt), FLinearColor(1.f, 1.f, 1.f, 0.75f));

	// --- Kontroller (alt) ---
	float BY = Canvas->ClipY - 22.f * TextScale * 3.f - 16.f;
	const FLinearColor Dim(1.f, 1.f, 1.f, 0.8f);
	Line(X, BY, TEXT("1-4 slot   Fare: hayalet   Sol tık: koy   R: döndür   Sağ tık / Esc: bırak"), Dim);
	Line(X, BY, TEXT("Backspace: son nesneyi geri al   F5: baştan   G: ızgara hep açık   H: bu panel"), Dim);
	Line(X, BY, TEXT("Konsol (~):  SLKnobs = ayarları listele   SLSet PlatformMass 10 = değiştir   SLSelfTest"), Dim);
}
