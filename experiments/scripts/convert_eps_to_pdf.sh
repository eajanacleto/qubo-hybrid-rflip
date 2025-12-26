#!/bin/bash
# Script para converter arquivos EPS para PDF

echo "Convertendo arquivos EPS para PDF..."

for eps_file in *.eps; do
    if [ -f "$eps_file" ]; then
        pdf_file="${eps_file%.eps}.pdf"
        echo "  $eps_file -> $pdf_file"
        ps2pdf -dEPSCrop "$eps_file" "$pdf_file"
    fi
done

echo ""
echo "Conversão concluída!"
echo ""
echo "Arquivos PDF gerados:"
ls -lh *.pdf 2>/dev/null || echo "Nenhum arquivo PDF encontrado"
