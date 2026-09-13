<#
.SYNOPSIS
Find untranslated preference strings in XUI files.

Copyright (c) 2025-2026, Allyn Viewer Contributors.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#>

[System.Console]::OutputEncoding = [System.Text.UTF8Encoding]::new($false)

$BaseDir = "D:\AllynViewer\indra\newview\skins\default\xui"
$EnDir = "$BaseDir\en-us"
$Languages = @("de", "es", "fr", "it", "pt")
$PrefFiles = @(
    "floater_preferences.xml",
    "panel_preferences_general.xml",
    "panel_preferences_graphics1.xml",
    "panel_preferences_audio.xml",
    "panel_preferences_voice.xml",
    "panel_preferences_network.xml",
    "panel_preferences_chat.xml",
    "panel_preferences_im.xml",
    "panel_preferences_popups.xml",
    "panel_preferences_skins.xml",
    "panel_preferences_input.xml",
    "panel_preferences_web.xml",
    "panel_preferences_translation.xml",
    "panel_preferences_grids.xml",
    "panel_preferences_ascent_chat.xml",
    "panel_preferences_ascent_vanity.xml",
    "panel_preferences_ascent_system.xml"
)

$TranslatableAttrs = @("label", "tool_tip", "tool_tip_checked", "title", "text")

function Get-TranslatableStrings {
    param([xml]$XmlDoc)
    $resultList = New-Object System.Collections.ArrayList
    $idx = @{}
    Walk $XmlDoc.DocumentElement "" $resultList $idx
    return $resultList
}

function Walk {
    param($Node, $ParentKey, $resultList, $idx)
    $tag = $Node.Name
    $name = $Node.GetAttribute("name")
    if ($name) {
        $key = "$tag`:$name"
    } else {
        if (-not $idx.ContainsKey($tag)) { $idx[$tag] = 0 }
        $idx[$tag]++
        $key = "$tag`:#$($idx[$tag])"
    }
    $fullKey = if ($ParentKey) { "$ParentKey/$key" } else { $key }

    foreach ($ta in $TranslatableAttrs) {
        if ($Node.HasAttribute($ta)) {
            [void]$resultList.Add([PSCustomObject]@{
                Key = $fullKey; Tag = $tag; Name = $name
                Type = "@$ta"; Value = $Node.GetAttribute($ta)
            })
        }
    }
    if ($tag -eq "string" -and $Node.HasAttribute("value")) {
        [void]$resultList.Add([PSCustomObject]@{
            Key = $fullKey; Tag = $tag; Name = $name
            Type = "@value"; Value = $Node.GetAttribute("value")
        })
    }

    $textVal = ""
    $hasText = $false
    if ($Node.ChildNodes.Count -eq 1 -and $Node.ChildNodes[0] -is [System.Xml.XmlText]) {
        $tv = $Node.ChildNodes[0].Value
        if ($tv -and $tv.Trim()) { $hasText = $true; $textVal = $tv.Trim() }
    } elseif ($Node.ChildNodes.Count -gt 0) {
        $allText = $true
        $tv = ""
        foreach ($cn in $Node.ChildNodes) {
            if ($cn -is [System.Xml.XmlElement]) { $allText = $false; break }
            if ($cn -is [System.Xml.XmlText]) { $tv += $cn.Value }
        }
        if ($allText -and $tv -and $tv.Trim()) { $hasText = $true; $textVal = $tv.Trim() }
    }
    if ($hasText) {
        [void]$resultList.Add([PSCustomObject]@{
            Key = $fullKey; Tag = $tag; Name = $name
            Type = "text"; Value = $textVal
        })
    }

    foreach ($child in $Node.ChildNodes) {
        if ($child -is [System.Xml.XmlElement]) { Walk $child $fullKey $resultList $idx }
    }
}

Write-Host "Loading English files..." -ForegroundColor Cyan
$enData = @{}
foreach ($file in $PrefFiles) {
    $enPath = "$EnDir\$file"
    if (-not (Test-Path $enPath)) { Write-Warning "English file missing: $enPath"; continue }
    try {
        $xml = New-Object System.Xml.XmlDocument
        $xml.Load($enPath)
        $enData[$file] = Get-TranslatableStrings $xml
    } catch { Write-Warning "Error loading $file : $_" }
}

$langStats = @{}
foreach ($lang in $Languages) { $langStats[$lang] = @{ Total = 0; Files = @{} } }

foreach ($lang in $Languages) {
    $langDir = "$BaseDir\$lang"
    Write-Host "`n========================================" -ForegroundColor Yellow
    Write-Host "  LANGUAGE: $lang" -ForegroundColor Yellow
    Write-Host "========================================" -ForegroundColor Yellow
    $totalUntranslated = 0

    foreach ($file in $PrefFiles) {
        $langPath = "$langDir\$file"
        $enItems = $enData[$file]
        if (-not $enItems) { continue }

        if (-not (Test-Path $langPath)) {
            Write-Host "  [$file] - FILE MISSING!"
            Write-Host "    Entire file: $($enItems.Count) untranslated strings"
            $totalUntranslated += $enItems.Count
            $langStats[$lang].Files[$file] = @{ Count = $enItems.Count; Items = $enItems }
            continue
        }

        try {
            $xml = New-Object System.Xml.XmlDocument
            $xml.Load($langPath)
            $langItems = Get-TranslatableStrings $xml
        } catch {
            Write-Host "  [$file] - PARSE ERROR!"
            $totalUntranslated += $enItems.Count
            $langStats[$lang].Files[$file] = @{ Count = $enItems.Count; Items = $enItems }
            continue
        }

        $langLookup = @{}
        foreach ($li in $langItems) {
            $lk = ($li.Key + "|" + $li.Type).ToLowerInvariant()
            if (-not $langLookup.ContainsKey($lk)) { $langLookup[$lk] = $li }
        }

        $fileUntranslated = New-Object System.Collections.ArrayList
        foreach ($enItem in $enItems) {
            $lookupKey = ($enItem.Key + "|" + $enItem.Type).ToLowerInvariant()
            $matched = $langLookup[$lookupKey]
            if ((-not $matched) -or ($matched.Value -eq $enItem.Value)) {
                [void]$fileUntranslated.Add($enItem)
            }
        }

        $count = $fileUntranslated.Count
        if ($count -gt 0) {
            Write-Host "  [$file] - $count untranslated"
            foreach ($u in $fileUntranslated) {
                Write-Host "    [$($u.Tag)/$($u.Name)] $($u.Type) = `"$($u.Value)`""
            }
            $langStats[$lang].Files[$file] = @{ Count = $count; Items = $fileUntranslated }
            $totalUntranslated += $count
        } else {
            Write-Host "  [$file] - OK"
            $langStats[$lang].Files[$file] = @{ Count = 0; Items = @() }
        }
    }

    Write-Host "`n  >>> TOTAL for $lang : $totalUntranslated <<<" -ForegroundColor Green
    $langStats[$lang].Total = $totalUntranslated
}

Write-Host "`n==========================================" -ForegroundColor Cyan
Write-Host "  FINAL SUMMARY" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan
$grandTotal = 0
foreach ($lang in $Languages) {
    $t = $langStats[$lang].Total
    Write-Host "  $lang : $t untranslated"
    $grandTotal += $t
}
Write-Host "  ------------------------"
Write-Host "  GRAND TOTAL: $grandTotal untranslated strings across all languages"
