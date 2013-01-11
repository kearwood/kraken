//
//  KRMaterialManager.cpp
//  KREngine
//
//  Copyright 2012 Kearwood Gilbert. All rights reserved.
//  
//  Redistribution and use in source and binary forms, with or without modification, are
//  permitted provided that the following conditions are met:
//  
//  1. Redistributions of source code must retain the above copyright notice, this list of
//  conditions and the following disclaimer.
//  
//  2. Redistributions in binary form must reproduce the above copyright notice, this list
//  of conditions and the following disclaimer in the documentation and/or other materials
//  provided with the distribution.
//  
//  THIS SOFTWARE IS PROVIDED BY KEARWOOD GILBERT ''AS IS'' AND ANY EXPRESS OR IMPLIED
//  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
//  FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL KEARWOOD GILBERT OR
//  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
//  ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
//  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  
//  The views and conclusions contained in the software and documentation are those of the
//  authors and should not be interpreted as representing official policies, either expressed
//  or implied, of Kearwood Gilbert.
//

#include "KREngine-common.h"
#include "KRMaterialManager.h"


KRMaterialManager::KRMaterialManager(KRContext &context, KRTextureManager *pTextureManager, KRShaderManager *pShaderManager) : KRContextObject(context)
{
    m_pTextureManager = pTextureManager;
    m_pShaderManager = pShaderManager;
}

KRMaterialManager::~KRMaterialManager() {

}


KRMaterial *KRMaterialManager::getMaterial(const char *szName) {
    std::string lowerName = szName;
    std::transform(lowerName.begin(), lowerName.end(),
                   lowerName.begin(), ::tolower);
    
    
    map<std::string, KRMaterial *>::iterator itr = m_materials.find(lowerName);
    if(itr == m_materials.end()) {
        fprintf(stderr, "Material not found: %s\n", szName);
        // Not found
        return NULL;
    } else {
        return (*itr).second;
    }
}
bool KRMaterialManager::load(const char *szName, KRDataBlock *data) {
    KRMaterial *pMaterial = NULL;
    char szSymbol[16][256];
    
 
    char *pScan = (char *)data->getStart();
    char *pEnd = (char *)data->getEnd();
    while(pScan < pEnd) {
    
        // Scan through whitespace
        while(pScan < pEnd && (*pScan == ' ' || *pScan == '\t' || *pScan == '\r' || *pScan == '\n')) {
            pScan++;
        }
        
        if(*pScan == '#') {
            // Line is a comment line
            
            // Scan to the end of the line
            while(pScan < pEnd && *pScan != '\r' && *pScan != '\n') {
                pScan++;
            }
        } else {
            int cSymbols = 0;
            while(pScan < pEnd && *pScan != '\n' && *pScan != '\r') {
                
                char *pDest = szSymbol[cSymbols++];
                while(pScan < pEnd && *pScan != ' ' && *pScan != '\n' && *pScan != '\r') {
                    if(*pScan >= 'A' && *pScan <= 'Z') {
                        *pDest++ = *pScan++ + 'a' - 'A'; // convert to lower case for case sensitve comparison later
                    } else {
                        *pDest++ = *pScan++;
                    }
                }
                *pDest = '\0';
                
                // Scan through whitespace, but don't advance to next line
                while(pScan < pEnd && (*pScan == ' ' || *pScan == '\t')) {
                    pScan++;
                }
            }
            
            if(cSymbols > 0) {
                
                if(strcmp(szSymbol[0], "newmtl") == 0 && cSymbols >= 2) {
                    
                    pMaterial = new KRMaterial(*m_pContext, szSymbol[1]);
                    m_materials[szSymbol[1]] = pMaterial;
                }
                if(pMaterial != NULL) {
                    if(strcmp(szSymbol[0], "alpha_mode") == 0) {
                        if(cSymbols == 2) {
                            if(strcmp(szSymbol[1], "test") == 0) {
                                pMaterial->setAlphaMode(KRMaterial::KRMATERIAL_ALPHA_MODE_TEST);
                            } else if(strcmp(szSymbol[1], "blendoneside") == 0) {
                                pMaterial->setAlphaMode(KRMaterial::KRMATERIAL_ALPHA_MODE_BLENDONESIDE);
                            } else if(strcmp(szSymbol[1], "blendtwoside") == 0) {
                                pMaterial->setAlphaMode(KRMaterial::KRMATERIAL_ALPHA_MODE_BLENDONESIDE);
                            } else {
                                pMaterial->setAlphaMode(KRMaterial::KRMATERIAL_ALPHA_MODE_OPAQUE);
                            }
                        }
                    } else if(strcmp(szSymbol[0], "ka") == 0) {
                        char *pScan2 = szSymbol[1];
                        float r = strtof(pScan2, &pScan2);
                        if(cSymbols == 2) {
                            pMaterial->setAmbient(KRVector3(r, r, r));
                        } else if(cSymbols == 4) {
                            pScan2 = szSymbol[2];
                            float g = strtof(pScan2, &pScan2);
                            pScan2 = szSymbol[3];
                            float b = strtof(pScan2, &pScan2);
                            pMaterial->setAmbient(KRVector3(r, g, b));
                        }
                    } else if(strcmp(szSymbol[0], "kd") == 0) {
                        char *pScan2 = szSymbol[1];
                        float r = strtof(pScan2, &pScan2);
                        if(cSymbols == 2) {
                            pMaterial->setDiffuse(KRVector3(r, r, r));
                        } else if(cSymbols == 4) {
                            pScan2 = szSymbol[2];
                            float g = strtof(pScan2, &pScan2);
                            pScan2 = szSymbol[3];
                            float b = strtof(pScan2, &pScan2);
                            pMaterial->setDiffuse(KRVector3(r, g, b));
                        }
                    } else if(strcmp(szSymbol[0], "ks") == 0) {
                        char *pScan2 = szSymbol[1];
                        float r = strtof(pScan2, &pScan2);
                        if(cSymbols == 2) {
                            pMaterial->setSpecular(KRVector3(r, r, r));
                        } else if(cSymbols == 4) {
                            pScan2 = szSymbol[2];
                            float g = strtof(pScan2, &pScan2);
                            pScan2 = szSymbol[3];
                            float b = strtof(pScan2, &pScan2);
                            pMaterial->setSpecular(KRVector3(r, g, b));
                        }
                    } else if(strcmp(szSymbol[0], "kr") == 0) {
                        char *pScan2 = szSymbol[1];
                        float r = strtof(pScan2, &pScan2);
                        if(cSymbols == 2) {
                            pMaterial->setReflection(KRVector3(r, r, r));
                        } else if(cSymbols == 4) {
                            pScan2 = szSymbol[2];
                            float g = strtof(pScan2, &pScan2);
                            pScan2 = szSymbol[3];
                            float b = strtof(pScan2, &pScan2);
                            pMaterial->setReflection(KRVector3(r, g, b));
                        }
                    } else if(strcmp(szSymbol[0], "tr") == 0) {
                        char *pScan2 = szSymbol[1];
                        float a = strtof(pScan2, &pScan2);
                        pMaterial->setTransparency(a);
                    } else if(strcmp(szSymbol[0], "ns") == 0) {
                        char *pScan2 = szSymbol[1];
                        float a = strtof(pScan2, &pScan2);
                        pMaterial->setShininess(a);
                    } else if(strncmp(szSymbol[0], "map", 3) == 0) {
                        // Truncate file extension
                        char *pScan2 = szSymbol[1];
                        char *pLastPeriod = NULL;
                        while(*pScan2 != '\0') {
                            if(*pScan2 == '.') {
                                pLastPeriod = pScan2;
                            }
                            pScan2++;
                        }
                        if(pLastPeriod) {
                            *pLastPeriod = '\0';
                        }

                        KRVector2 texture_scale = KRVector2(1.0f, 1.0f);
                        KRVector2 texture_offset = KRVector2(0.0f, 0.0f);
                        
                        int iScanSymbol = 2;
                        int iScaleParam = -1;
                        int iOffsetParam = -1;
                        while(iScanSymbol < cSymbols) {
                            if(strcmp(szSymbol[iScanSymbol], "-s") == 0) {
                                // Scale
                                iScaleParam = 0;
                                iOffsetParam = -1;
                            } else if(strcmp(szSymbol[iScanSymbol], "-o") == 0) {
                                // Offset
                                iOffsetParam = 0;
                                iScaleParam = -1;
                            } else {
                                char *pScan3 = szSymbol[iScanSymbol];
                                float v = strtof(pScan3, &pScan3);
                                if(iScaleParam == 0) {
                                    texture_scale.x = v;
                                    iScaleParam++;
                                } else if(iScaleParam == 1) {
                                    texture_scale.y = v;
                                    iScaleParam++;
                                } else if(iOffsetParam == 0) {
                                    texture_offset.x = v;
                                    iOffsetParam++;
                                } else if(iOffsetParam == 1) {
                                    texture_offset.y = v;
                                    iOffsetParam++;
                                }
                            }
                            iScanSymbol++;
                        }

                        if(strcmp(szSymbol[0], "map_ka") == 0) {
                            pMaterial->setAmbientMap(szSymbol[1], texture_scale, texture_offset);
                        } else if(strcmp(szSymbol[0], "map_kd") == 0) {
                            pMaterial->setDiffuseMap(szSymbol[1], texture_scale, texture_offset);
                        } else if(strcmp(szSymbol[0], "map_ks") == 0) {
                            pMaterial->setSpecularMap(szSymbol[1], texture_scale, texture_offset);
                        } else if(strcmp(szSymbol[0], "map_normal") == 0) {
                            pMaterial->setNormalMap(szSymbol[1], texture_scale, texture_offset);
                        } else if(strcmp(szSymbol[0], "map_reflection") == 0) {
                            pMaterial->setReflectionMap(szSymbol[1], texture_scale, texture_offset);
                        } else if(strcmp(szSymbol[0], "map_reflectioncube") == 0) {
                            pMaterial->setReflectionCube(szSymbol[1]);
                        }
                    }
                }
            }
        }

    }

    delete data;
    return true;
}
