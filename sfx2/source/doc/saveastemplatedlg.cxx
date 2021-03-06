/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <sfx2/saveastemplatedlg.hxx>

#include <comphelper/processfactory.hxx>
#include <comphelper/string.hxx>
#include <comphelper/storagehelper.hxx>
#include <sfx2/sfxresid.hxx>
#include <sfx2/app.hxx>
#include <sfx2/fcontnr.hxx>
#include <sfx2/docfac.hxx>
#include <sfx2/doctempl.hxx>
#include <sfx2/docfilt.hxx>
#include <vcl/edit.hxx>
#include <vcl/lstbox.hxx>
#include <vcl/weld.hxx>
#include <sot/storage.hxx>

#include <com/sun/star/frame/DocumentTemplates.hpp>
#include <com/sun/star/frame/XStorable.hpp>

#include <sfx2/strings.hrc>

using namespace ::com::sun::star;
using namespace ::com::sun::star::frame;

// Class SfxSaveAsTemplateDialog --------------------------------------------------

SfxSaveAsTemplateDialog::SfxSaveAsTemplateDialog():
        ModalDialog(nullptr, "SaveAsTemplateDialog", "sfx/ui/saveastemplatedlg.ui"),
        msSelectedCategory(OUString()),
        msTemplateName(OUString()),
        mnRegionPos(0)
{
    get(mpLBCategory, "categorylb");
    get(mpCBXDefault, "defaultcb");
    get(mpTemplateNameEdit, "name_entry");
    get(mpOKButton, "ok");

    initialize();
    SetCategoryLBEntries(msCategories);

    mpTemplateNameEdit->SetModifyHdl(LINK(this, SfxSaveAsTemplateDialog, TemplateNameEditHdl));
    mpLBCategory->SetSelectHdl(LINK(this, SfxSaveAsTemplateDialog, SelectCategoryHdl));
    mpOKButton->SetClickHdl(LINK(this, SfxSaveAsTemplateDialog, OkClickHdl));

    mpOKButton->Disable();
    mpOKButton->SetText(SfxResId(STR_SAVEDOC));
}

SfxSaveAsTemplateDialog::~SfxSaveAsTemplateDialog()
{
    disposeOnce();
}

void SfxSaveAsTemplateDialog::dispose()
{
    mpLBCategory.clear();
    mpTemplateNameEdit.clear();
    mpOKButton.clear();
    mpCBXDefault.clear();

    ModalDialog::dispose();
}

void SfxSaveAsTemplateDialog::setDocumentModel(const uno::Reference<frame::XModel> &rModel)
{
    m_xModel = rModel;
}

IMPL_LINK_NOARG(SfxSaveAsTemplateDialog, OkClickHdl, Button*, void)
{
    std::unique_ptr<weld::MessageDialog> xQueryDlg(Application::CreateMessageDialog(GetFrameWeld(), VclMessageType::Question, VclButtonsType::YesNo,
                                                   OUString()));
    if(!IsTemplateNameUnique())
    {
        OUString sQueryMsg(SfxResId(STR_QMSG_TEMPLATE_OVERWRITE));
        sQueryMsg = sQueryMsg.replaceFirst("$1",msTemplateName);
        xQueryDlg->set_primary_text(sQueryMsg.replaceFirst("$2", msSelectedCategory));

        if (xQueryDlg->run() == RET_NO)
            return;
    }

    if(SaveTemplate())
        Close();
    else
    {
        OUString sText( SfxResId(STR_ERROR_SAVEAS) );
        std::unique_ptr<weld::MessageDialog> xBox(Application::CreateMessageDialog(GetFrameWeld(), VclMessageType::Warning, VclButtonsType::Ok,
                                                  sText.replaceFirst("$1", msTemplateName)));
        xBox->run();
    }
}

IMPL_LINK_NOARG(SfxSaveAsTemplateDialog, TemplateNameEditHdl, Edit&, void)
{
    msTemplateName = comphelper::string::strip(mpTemplateNameEdit->GetText(), ' ');
    SelectCategoryHdl(*mpLBCategory);
}

IMPL_LINK_NOARG(SfxSaveAsTemplateDialog, SelectCategoryHdl, ListBox&, void)
{
    if(mpLBCategory->GetSelectedEntryPos() == 0)
    {
        msSelectedCategory = OUString();
        mpOKButton->Disable();
    }
    else
    {
        msSelectedCategory = mpLBCategory->GetSelectedEntry();
        mpOKButton->Enable(!msTemplateName.isEmpty());
    }
}

void SfxSaveAsTemplateDialog::initialize()
{
    sal_uInt16 nCount = maDocTemplates.GetRegionCount();
    for (sal_uInt16 i = 0; i < nCount; ++i)
    {
        OUString sCategoryName(maDocTemplates.GetFullRegionName(i));
        msCategories.push_back(sCategoryName);
    }
}

void SfxSaveAsTemplateDialog::SetCategoryLBEntries(std::vector<OUString> aFolderNames)
{
    if (!aFolderNames.empty())
    {
        for (size_t i = 0, n = aFolderNames.size(); i < n; ++i)
            mpLBCategory->InsertEntry(aFolderNames[i], i+1);
    }
    mpLBCategory->SelectEntryPos(0);
}

bool SfxSaveAsTemplateDialog::IsTemplateNameUnique()
{
    std::vector<OUString>::iterator it;
    it=find(msCategories.begin(), msCategories.end(), msSelectedCategory);
    mnRegionPos = std::distance(msCategories.begin(), it);

    sal_uInt16 nEntries = maDocTemplates.GetCount(mnRegionPos);
    for(sal_uInt16 i = 0; i < nEntries; i++)
    {
        OUString aName = maDocTemplates.GetName(mnRegionPos, i);
        if(aName == msTemplateName)
            return false;
    }

    return true;
}

bool SfxSaveAsTemplateDialog::SaveTemplate()
{
    uno::Reference< frame::XStorable > xStorable(m_xModel, uno::UNO_QUERY_THROW );

    uno::Reference< frame::XDocumentTemplates > xTemplates(frame::DocumentTemplates::create(comphelper::getProcessComponentContext()) );

    if (!xTemplates->storeTemplate( msSelectedCategory, msTemplateName, xStorable ))
        return false;

    sal_uInt16 nDocId = maDocTemplates.GetCount(mnRegionPos);
    OUString     sURL = maDocTemplates.GetTemplateTargetURLFromComponent(msSelectedCategory, msTemplateName);
    bool bIsSaved = maDocTemplates.InsertTemplate( mnRegionPos, nDocId, msTemplateName, sURL);

    if (!bIsSaved)
        return false;

    if ( !sURL.isEmpty() && mpCBXDefault->IsChecked() )
    {
        OUString aServiceName;
        try
        {
            uno::Reference< embed::XStorage > xStorage =
                    comphelper::OStorageHelper::GetStorageFromURL( sURL, embed::ElementModes::READ );

            SotClipboardFormatId nFormat = SotStorage::GetFormatID( xStorage );

            std::shared_ptr<const SfxFilter> pFilter = SfxGetpApp()->GetFilterMatcher().GetFilter4ClipBoardId( nFormat );

            if ( pFilter )
                aServiceName = pFilter->GetServiceName();
        }
        catch( uno::Exception& )
        {}

        if(!aServiceName.isEmpty())
            SfxObjectFactory::SetStandardTemplate(aServiceName, sURL);
    }

    maDocTemplates.Update();
    return true;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
