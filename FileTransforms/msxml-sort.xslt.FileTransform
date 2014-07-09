<xsl:stylesheet version='1.0' xmlns:xsl='http://www.w3.org/1999/XSL/Transform'>
	<xsl:output method='xml' encoding='UTF-8' indent='no' omit-xml-declaration='yes'/>
	<xsl:template match='node()|@*'>
		<xsl:copy>
			<xsl:apply-templates select='node()|@*'>
				<xsl:sort select='@*'/>
			</xsl:apply-templates>
		</xsl:copy>
	</xsl:template>
</xsl:stylesheet>
