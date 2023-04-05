/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   get_next_line.h                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ntaleb <ntaleb@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/08/14 14:28:21 by noureddine        #+#    #+#             */
/*   Updated: 2022/10/29 09:29:42 by ntaleb           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef GET_NEXT_LINE_H
# define GET_NEXT_LINE_H

# include <stdlib.h>

char	*get_next_line(int fd);
char	*ft_strchr(const char *s, int c);
void	*ft_memmove(void *dst, const void *src, size_t len);
size_t	ft_strlen(const char *str);
char	*ft_strdup(char *s);
char	*ft_strjoin(const char *s1, const char *s2);

#endif