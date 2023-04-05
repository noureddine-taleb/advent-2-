/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   get_next_line_utils.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ntaleb <ntaleb@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/08/14 15:09:02 by noureddine        #+#    #+#             */
/*   Updated: 2022/10/29 09:29:28 by ntaleb           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdlib.h>
#include "get_next_line.h"

size_t	ft_strlen(const char *str)
{
	int	len;

	len = 0;
	while (*str++)
		len++;
	return (len);
}

char	*ft_strchr(const char *s, int c)
{
	while (*s)
	{
		if (*s == (char)c)
			return ((char *)s);
		s++;
	}
	if (*s == c)
		return ((char *)s);
	return ((char *)0);
}

void	*ft_memmove(void *dst, const void *src, size_t len)
{
	char		*ret;

	ret = dst;
	if ((!dst && !src) || len == 0)
		return (ret);
	while (len--)
		*(char *)dst++ = *(char *)src++;
	return (ret);
}

char	*ft_strdup(char *s)
{
	char	*str;
	char	*ret;
	size_t	len;

	len = ft_strlen(s) + 1;
	str = malloc(len);
	if (!str)
		return (NULL);
	ret = str;
	while (*s)
		*str++ = *s++;
	*str = 0;
	return (ret);
}

char	*ft_strjoin(const char *s1, const char *s2)
{
	unsigned int	tlen;
	char			*str;
	char			*ret;

	if (!s1 || !s2)
		return (NULL);
	tlen = ft_strlen(s1) + ft_strlen(s2) + 1;
	str = malloc(tlen);
	if (!str)
		return (NULL);
	*str = 0;
	ret = str;
	while (*s1)
		*str++ = *s1++;
	while (*s2)
		*str++ = *s2++;
	*str = 0;
	return (ret);
}
